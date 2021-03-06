﻿#include "stdafx.h"
#include <string>
#include <list>
#include <map>
#include <unordered_map>
#include <regex>
#include <iostream>
#include <sstream>
#include "Decompiler.h"
#include "RawEclDecoder.h"

using namespace std;
using namespace DecompilerInternal;

namespace DecompilerInternal {
	struct RootDecompileContext final {
		/// <summary>
		/// A map from subroutine IDs to the formal parameter list determined by calls to the subroutine.
		/// </summary>
		unordered_map<string, vector<Op::mType>> map_subv;
	};

	struct SubDecompileContext final {
		shared_ptr<BlockSeqBlock> blk_root;
	};

	struct SubDecompilePhaseCContext final {
		/// <summary>A map from pointers to second-level sequence blocks to their iterators in <c>blk_root->list_blk_child</c>.</summary>
		// TODO: Remove?
		unordered_map<const BlockSeqBlock*, list<shared_ptr<Block>>::const_iterator> map_it_blk_seq;
		/// <summary>A set containing ancestors of the block currently being iterated.</summary>
		unordered_set<const BlockSeqBlock*> set_ance;
	};

	struct SubDecompilePhaseCYield final {
		enum YieldType {
			Undefined,
			/// <summary>An exit from the subroutine is found within this iteration.</summary>
			ExitSubFound,
			/// <summary>A loop is found within this iteration.</summary>
			LoopFound
		};
		YieldType yield_type = Undefined;
		/// <summary>
		/// The chain of blocks from the current block to the block that caused the yield.
		/// Only available for <c>YieldType::LoopFound</c>.
		/// </summary>
		shared_ptr<list<shared_ptr<BlockSeqBlock>>> chain;
	};

	/// <summary>A <c>Block</c> represents a body of code, possibly containing control structures.</summary>
	struct Block abstract {
		enum BlockType {
			BlockSeq,
			JmpTarget,
			SingleIns
		};
		const BlockType block_type;
		explicit Block(BlockType block_type) : block_type(block_type) {}
		virtual ~Block() {}
		/// <summary>Get the time value when entering this block.</summary>
		virtual uint32_t GetEnterTime() const = 0;
		/// <summary>Whether all instructions in this block has the same time value.</summary>
		virtual bool IsConstTime() const = 0;
	};

	/// <summary>A <c>Block</c> that represents a sequence of child blocks.</summary>
	struct BlockSeqBlock final : public Block {
		enum ExitType {
			/// <summary>No exit defined.</summary>
			Undefined,
			/// <summary>Fallthrough to the next block. <c>vec_exit[0]</c>: The next block.</summary>
			Fallthrough,
			/// <summary>Goto target block. <c>vec_exit[0]</c>: The target block.</summary>
			Goto,
			/// <summary>Goto target block if zero, fallthrough otherwise. <c>vec_exit[0]</c>: The target block. <c>vec_exit[1]</c>: The next block.</summary>
			GotoIfZero,
			/// <summary>Goto target block if nonzero, fallthrough otherwise. <c>vec_exit[0]</c>: The target block. <c>vec_exit[1]</c>: The next block.</summary>
			GotoIfNonZero,
			/// <summary>Return to caller.</summary>
			Return,
			/// <summary>Exit thread.</summary>
			ExitThread,
			/// <summary>Multiple exits may exist. <c>vec_exit[1]</c>: The next block.</summary>
			MultiExit
		};
		struct exit_def_t {
			shared_ptr<BlockSeqBlock> blk_exit;
			uint32_t time_exit;
			explicit exit_def_t(const shared_ptr<BlockSeqBlock>& blk_exit, uint32_t time_exit) : blk_exit(blk_exit), time_exit(time_exit) {}
		};
		static BlockSeqBlock* CastToMe(Block* p) {
			if (p->block_type != BlockType::BlockSeq) throw(ErrDesignApp("BlockSeqBlock::CastToMe : p->block_type != BlockType::BlockSeq"));
			BlockSeqBlock* ret = dynamic_cast<BlockSeqBlock*>(p);
			if (!ret) throw(ErrDesignApp("BlockSeqBlock::CastToMe : cannot cast p to type \"BlockSeqBlock*\""));
			return ret;
		}
		list<shared_ptr<Block>> list_blk_child;
		ExitType exit_type = ExitType::Undefined;
		/// <summary>A vector that stores exits to other blocks. Depends on <c>exit_type</c>.</summary>
		vector<exit_def_t> vec_exit;
		BlockSeqBlock() : Block(Block::BlockType::BlockSeq) {}
		virtual ~BlockSeqBlock() {}
		virtual uint32_t GetEnterTime() const override {
			uint32_t time = UINT32_MAX;
			for (const shared_ptr<Block>& val_blk_child : this->list_blk_child) {
				uint32_t time_child = val_blk_child->GetEnterTime();
				if (time_child != UINT32_MAX) {
					time = time_child;
					break;
				}
			}
			return time;
		}
		virtual bool IsConstTime() const override {
			uint32_t time = UINT32_MAX;
			for (const shared_ptr<Block>& val_blk_child : this->list_blk_child) {
				if (!val_blk_child->IsConstTime()) return false;
				uint32_t time_child = val_blk_child->GetEnterTime();
				if (time_child != UINT32_MAX) {
					if (time != UINT32_MAX && time != time_child) return false;
					time = time_child;
					break;
				}
			}
			return true;
		}
	};

	/// <summary>A <c>Block</c> that represents a jump target.</summary>
	struct JmpTargetBlock final : public Block {
		static JmpTargetBlock* CastToMe(Block* p) {
			if (p->block_type != BlockType::JmpTarget) throw(ErrDesignApp("JmpTargetBlock::CastToMe : p->block_type != BlockType::JmpTarget"));
			JmpTargetBlock* ret = dynamic_cast<JmpTargetBlock*>(p);
			if (!ret) throw(ErrDesignApp("JmpTargetBlock::CastToMe : cannot cast p to type \"JmpTargetBlock*\""));
			return ret;
		}
		uint32_t id_target = UINT32_MAX;
		JmpTargetBlock() : Block(Block::BlockType::JmpTarget) {}
		virtual ~JmpTargetBlock() {}
		virtual uint32_t GetEnterTime() const override { return UINT32_MAX; }
		virtual bool IsConstTime() const override { return true; }
	};

	/// <summary>A <c>Block</c> that represents a single instruction.</summary>
	struct SingleInsBlock final : public Block {
		static SingleInsBlock* CastToMe(Block* p) {
			if (p->block_type != BlockType::SingleIns) throw(ErrDesignApp("SingleInsBlock::CastToMe : p->block_type != BlockType::SingleIns"));
			SingleInsBlock* ret = dynamic_cast<SingleInsBlock*>(p);
			if (!ret) throw(ErrDesignApp("SingleInsBlock::CastToMe : cannot cast p to type \"SingleInsBlock*\""));
			return ret;
		}
		shared_ptr<DecodedIns> ins;
		SingleInsBlock() : Block(Block::BlockType::SingleIns) {}
		virtual ~SingleInsBlock() {}
		virtual uint32_t GetEnterTime() const override { return this->ins ? this->ins->time : UINT32_MAX; }
		virtual bool IsConstTime() const override { return true; }
	};
}

const set<uint16_t> Decompiler::set_ins_controlflow = { 1, 10, 12, 13, 14 };

Decompiler::Decompiler() {}

Decompiler::~Decompiler() {}

void Decompiler::DecompileRoot(const DecodedRoot& root, ostream& stream) {
	regex rgx_find_quote("\\\"", regex::flag_type::optimize);
	string str_rpl_quote("\\\"");
	for (const string& val_anim : root.anim) {
		stream << "#anim \"" << regex_replace(val_anim, rgx_find_quote, str_rpl_quote, regex_constants::match_flag_type::match_default) << "\"\n";
	}
	for (const string& val_ecli : root.anim) {
		stream << "#ecli \"" << regex_replace(val_ecli, rgx_find_quote, str_rpl_quote, regex_constants::match_flag_type::match_default) << "\"\n";
	}
	stream << "\n";

	RootDecompileContext root_ctx;

	for (const shared_ptr<DecodedSub>& val_sub : root.subs) {
		for (const shared_ptr<DecodedSubDataEntry>& val_data_entry : val_sub->data_entries) {
			if (val_data_entry->data_entry_type == DecodedSubDataEntry::DataEntryType::Ins) {
				const DecodedIns* ins = DecodedIns::CastToMe(val_data_entry.get());
				if (ins->id == 11 || ins->id == 15) {
					if (ins->params.size() < 1) throw(ErrDesignApp("Decompiler::DecompileRoot : too few parameters for a call instruction"));
					const DecodedParam_String* param_sub_name = DecodedParam_String::CastToMe(ins->params.at(0).get());
					vector<Op::mType> vec_subv;
					for (size_t i_param = 1; i_param < ins->params.size(); ++i_param) {
						if (DecodedParam_Call::CastToMe(ins->params.at(i_param).get())->is_to_float) {
							vec_subv.push_back(Op::mType::Float);
						} else {
							vec_subv.push_back(Op::mType::Int);
						}
					}
					try {
						if (root_ctx.map_subv.at(param_sub_name->val) != vec_subv) {
							cerr << "Decompiler::DecompileRoot : Warning: inconsistent parameter lists when calling sub " << param_sub_name->val << "\n";
						}
					} catch (out_of_range&) {
						root_ctx.map_subv[param_sub_name->val] = vec_subv;
					}
				}
			}
		}
	}

	for (const shared_ptr<DecodedSub>& val_sub : root.subs) {
		stream << "sub " << val_sub->id << "(";
		const vector<Op::mType>* subv = nullptr;
		map<uint32_t, string> map_var_name;
		try {
			subv = &root_ctx.map_subv.at(val_sub->id);
		} catch (out_of_range&) {}
		if (subv) {
			uint32_t i_subv = 0;
			uint32_t id_var_next = 0;
			for (Op::mType val_subv : *subv) {
				if (i_subv) stream << ", ";
				switch (val_subv) {
				case Op::mType::Int: {
					string str_var_name = "param_";
					ostringstream(str_var_name) << i_subv;
					stream << "int " << str_var_name;
					map_var_name[id_var_next++] = str_var_name;
					break;
				}
				case Op::mType::Float: {
					string str_var_name = "param_";
					ostringstream(str_var_name) << i_subv;
					stream << "float " << str_var_name;
					map_var_name[id_var_next++] = str_var_name;
					break;
				}
				default:
					throw(ErrDesignApp("Decompiler::DecompileRoot : unknown formal parameter type"));
				}
				++i_subv;
			}
		}
		stream << ") ";
		this->DecompileSub(root_ctx, *val_sub, map_var_name, stream);
		stream << "\n\n";
	}
}

void Decompiler::DecompileSub(RootDecompileContext& root_ctx, const DecodedSub& sub, map<uint32_t, string>& map_var_name, ostream& stream_stmts) {
	SubDecompileContext sub_ctx;
	BlockSeqBlock* blk_root = new BlockSeqBlock();
	sub_ctx.blk_root = shared_ptr<BlockSeqBlock>(blk_root);
	for (const shared_ptr<DecodedSubDataEntry>& val_data_entry : sub.data_entries) {
		switch (val_data_entry->data_entry_type) {
		case DecodedSubDataEntry::DataEntryType::JmpTarget: {
			JmpTargetBlock* blk_child = new JmpTargetBlock();
			blk_root->list_blk_child.push_back(shared_ptr<Block>(blk_child));
			blk_child->id_target = DecodedJmpTarget::CastToMe(val_data_entry.get())->id_target;
			break;
		}
		case DecodedSubDataEntry::DataEntryType::Ins: {
			SingleInsBlock* blk_child = new SingleInsBlock();
			blk_root->list_blk_child.push_back(shared_ptr<Block>(blk_child));
			blk_child->ins = dynamic_pointer_cast<DecodedIns, DecodedSubDataEntry>(val_data_entry);
			if (!blk_child->ins) throw(ErrDesignApp("Decompiler::DecompileSub : cannot cast val_data_entry to type \"DecodedIns*\""));
			break;
		}
		default:
			throw(ErrDesignApp("Decompiler::DecompileSub : unknown data entry type"));
		}
	}
	this->DecompileSubPhaseA(root_ctx, sub_ctx);
	this->DecompileSubPhaseB(root_ctx, sub_ctx);
	this->DecompileSubPostPhaseBCheck(root_ctx, sub_ctx);
	this->DecompileSubPhaseC(root_ctx, sub_ctx);
	// TODO: Implement Decompiler::DecompileSub.
}

void Decompiler::DecompileSubPhaseA(RootDecompileContext& root_ctx, SubDecompileContext& sub_ctx) {
	uint32_t time_prev = 0;
	list<shared_ptr<Block>>::iterator it_list_blk_child_splitbegin;
	list<shared_ptr<Block>>::iterator it_list_blk_child;
	it_list_blk_child_splitbegin = sub_ctx.blk_root->list_blk_child.begin();
	bool should_split_next = false;
	for (
		it_list_blk_child = sub_ctx.blk_root->list_blk_child.begin();
		it_list_blk_child != sub_ctx.blk_root->list_blk_child.end();
		) {
		bool should_split = false;
		if (should_split_next)
			should_split = true;
		should_split_next = false;
		switch ((*it_list_blk_child)->block_type) {
		case Block::BlockType::JmpTarget: {
			JmpTargetBlock* blk = JmpTargetBlock::CastToMe(it_list_blk_child->get());
			should_split = true;
			break;
		}
		case Block::BlockType::SingleIns: {
			SingleInsBlock* blk = SingleInsBlock::CastToMe(it_list_blk_child->get());
			if (blk->ins->time != time_prev)
				should_split = true;
			time_prev = blk->ins->time;
			if (set_ins_controlflow.count(blk->ins->id))
				should_split_next = true;
			break;
		}
		default:
			throw(ErrDesignApp("Decompiler::DecompileSubPhaseA : unknown block type"));
		}
		if (should_split) {
			list<shared_ptr<Block>>::iterator it_list_blk_child_splitend = it_list_blk_child++;
			BlockSeqBlock* blk_new = new BlockSeqBlock();
			blk_new->list_blk_child.splice(blk_new->list_blk_child.end(), sub_ctx.blk_root->list_blk_child, it_list_blk_child_splitbegin, it_list_blk_child_splitend);
			sub_ctx.blk_root->list_blk_child.insert(it_list_blk_child_splitend, shared_ptr<Block>(blk_new));
			it_list_blk_child_splitbegin = it_list_blk_child_splitend;
		} else {
			++it_list_blk_child;
		}
	}
	if (it_list_blk_child_splitbegin != sub_ctx.blk_root->list_blk_child.end()) {
		list<shared_ptr<Block>>::iterator it_list_blk_child_splitend = sub_ctx.blk_root->list_blk_child.end();
		BlockSeqBlock* blk_new = new BlockSeqBlock();
		blk_new->list_blk_child.splice(blk_new->list_blk_child.end(), sub_ctx.blk_root->list_blk_child, it_list_blk_child_splitbegin, it_list_blk_child_splitend);
		sub_ctx.blk_root->list_blk_child.insert(it_list_blk_child_splitend, shared_ptr<Block>(blk_new));
		it_list_blk_child_splitbegin = it_list_blk_child_splitend;
	}
}

void Decompiler::DecompileSubPhaseB(DecompilerInternal::RootDecompileContext& root_ctx, DecompilerInternal::SubDecompileContext& sub_ctx) {
	map<uint32_t, shared_ptr<BlockSeqBlock>> map_blk_target;
	// Eliminate jump targets and fill map_blk_target.
	for (const shared_ptr<Block>& val_blk_seq : sub_ctx.blk_root->list_blk_child) {
		BlockSeqBlock* blk_seq = BlockSeqBlock::CastToMe(val_blk_seq.get());
		// Search for a jump target in the sequence.
		list<shared_ptr<Block>>::iterator it_list_blk_child;
		for (it_list_blk_child = blk_seq->list_blk_child.begin();
			it_list_blk_child != blk_seq->list_blk_child.end();
			++it_list_blk_child)
			if (
				(*it_list_blk_child)->block_type == Block::BlockType::SingleIns
				|| (*it_list_blk_child)->block_type == Block::BlockType::JmpTarget
				) break;
		// If a jump target is found, add an entry in map_blk_target and delete the target.
		if (
			it_list_blk_child != blk_seq->list_blk_child.end()
			&& (*it_list_blk_child)->block_type == Block::BlockType::JmpTarget
			) {
			map_blk_target[JmpTargetBlock::CastToMe(it_list_blk_child->get())->id_target] = dynamic_pointer_cast<BlockSeqBlock, Block>(val_blk_seq);
			blk_seq->list_blk_child.erase(it_list_blk_child);
		}
	}
	// Set exit types and exits of the BlockSeqBlock blocks.
	for (list<shared_ptr<Block>>::iterator it_list_blk_seq = sub_ctx.blk_root->list_blk_child.begin();
		it_list_blk_seq != sub_ctx.blk_root->list_blk_child.end();
		++it_list_blk_seq) {
		BlockSeqBlock* blk_seq = BlockSeqBlock::CastToMe(it_list_blk_seq->get());
		shared_ptr<BlockSeqBlock> blk_seq_fallthrough;
		if (it_list_blk_seq != sub_ctx.blk_root->list_blk_child.end()) {
			list<shared_ptr<Block>>::const_iterator it_list_blk_seq_next = it_list_blk_seq;
			++it_list_blk_seq_next;
			if ((*it_list_blk_seq_next)->block_type != Block::BlockType::BlockSeq) throw(ErrDesignApp("Decompiler::DecompileSubPhaseB : a child block of blk_root isn't BlockSeqBlock after phase A"));
			blk_seq_fallthrough = dynamic_pointer_cast<BlockSeqBlock, Block>(*it_list_blk_seq_next);
			if (!blk_seq_fallthrough) throw(ErrDesignApp("Decompiler::DecompileSubPhaseB : cannot cast *it_list_blk_seq_next to type \"BlockSeqBlock*\""));
		}
		blk_seq->vec_exit.clear();
		// Search for the last instruction in the sequence.
		list<shared_ptr<Block>>::reverse_iterator rit_list_blk_child;
		for (rit_list_blk_child = blk_seq->list_blk_child.rbegin();
			rit_list_blk_child != blk_seq->list_blk_child.rend();
			++rit_list_blk_child)
			if ((*rit_list_blk_child)->block_type == Block::BlockType::SingleIns)
				break;
		if (
			rit_list_blk_child != blk_seq->list_blk_child.rend()
			&& (*rit_list_blk_child)->block_type == Block::BlockType::SingleIns
			) {
			SingleInsBlock* blk_child = SingleInsBlock::CastToMe(rit_list_blk_child->get());
			// If the last instruction is a control flow instruction, set the exit type and exit correspondingly and delete the last instruction.
			if (set_ins_controlflow.count(blk_child->ins->id)) {
				if (blk_child->ins->is_rawins) throw(ErrDesignApp("Decompiler::DecompileSubPhaseB : one of the control flow instructions is raw"));
				switch (blk_child->ins->id) {
				case 1: {
					blk_seq->exit_type = BlockSeqBlock::ExitType::ExitThread;
					break;
				}
				case 10: {
					blk_seq->exit_type = BlockSeqBlock::ExitType::Return;
					break;
				}
				case 12: {
					blk_seq->exit_type = BlockSeqBlock::ExitType::Goto;
					uint32_t id_target = DecodedParam_Jmp::CastToMe(blk_child->ins->params.at(0).get())->id_target;
					uint32_t time_exit_target = DecodedParam_Int::CastToMe(blk_child->ins->params.at(1).get())->val;
					blk_seq->vec_exit[0] = BlockSeqBlock::exit_def_t(map_blk_target.at(id_target), time_exit_target);
					break;
				}
				case 13: {
					blk_seq->exit_type = BlockSeqBlock::ExitType::GotoIfZero;
					uint32_t id_target = DecodedParam_Jmp::CastToMe(blk_child->ins->params.at(0).get())->id_target;
					uint32_t time_exit_target = DecodedParam_Int::CastToMe(blk_child->ins->params.at(1).get())->val;
					blk_seq->vec_exit[0] = BlockSeqBlock::exit_def_t(map_blk_target.at(id_target), time_exit_target);
					blk_seq->vec_exit[1] = BlockSeqBlock::exit_def_t(blk_seq_fallthrough, blk_seq->GetEnterTime());
					break;
				}
				case 14: {
					blk_seq->exit_type = BlockSeqBlock::ExitType::GotoIfNonZero;
					uint32_t id_target = DecodedParam_Jmp::CastToMe(blk_child->ins->params.at(0).get())->id_target;
					uint32_t time_exit_target = DecodedParam_Int::CastToMe(blk_child->ins->params.at(1).get())->val;
					blk_seq->vec_exit[0] = BlockSeqBlock::exit_def_t(map_blk_target.at(id_target), time_exit_target);
					blk_seq->vec_exit[1] = BlockSeqBlock::exit_def_t(blk_seq_fallthrough, blk_seq->GetEnterTime());
					break;
				}
				default:
					throw(ErrDesignApp("Decompiler::DecompileSubPhaseB : unknown control flow instruction"));
				}
				list<shared_ptr<Block>>::iterator it_list_blk_child = rit_list_blk_child.base();
				--it_list_blk_child;
				blk_seq->list_blk_child.erase(it_list_blk_child);
			} else {
				blk_seq->exit_type = BlockSeqBlock::ExitType::Fallthrough;
				blk_seq->vec_exit[0] = BlockSeqBlock::exit_def_t(blk_seq_fallthrough, blk_seq->GetEnterTime());
			}
		} else {
			blk_seq->exit_type = BlockSeqBlock::ExitType::Fallthrough;
			blk_seq->vec_exit[0] = BlockSeqBlock::exit_def_t(blk_seq_fallthrough, blk_seq->GetEnterTime());
		}
	}
}

void Decompiler::DecompileSubPostPhaseBCheck(DecompilerInternal::RootDecompileContext& root_ctx, DecompilerInternal::SubDecompileContext& sub_ctx) {
	for (const shared_ptr<Block>& blk_seq : sub_ctx.blk_root->list_blk_child) {
		if (!blk_seq->IsConstTime()) throw(ErrDesignApp("Decompiler::DecompileSubPostPhaseBCheck : !blk_seq->IsConstTime()"));
		if (blk_seq->block_type != Block::BlockType::BlockSeq) throw(ErrDesignApp("Decompiler::DecompileSubPostPhaseBCheck : blk_seq->block_type != Block::BlockType::BlockSeq"));
		for (const shared_ptr<Block>& blk_child : BlockSeqBlock::CastToMe(blk_seq.get())->list_blk_child) {
			if (blk_child->block_type != Block::BlockType::SingleIns) throw(ErrDesignApp("Decompiler::DecompileSubPostPhaseBCheck : blk_child->block_type != Block::BlockType::SingleIns"));
			if (set_ins_controlflow.count(SingleInsBlock::CastToMe(blk_child.get())->ins->id)) throw(ErrDesignApp("Decompiler::DecompileSubPostPhaseBCheck : set_ins_controlflow.count(SingleInsBlock::CastToMe(blk_child.get())->ins->id)"));
		}
	}
}

void Decompiler::DecompileSubPhaseC(DecompilerInternal::RootDecompileContext& root_ctx, DecompilerInternal::SubDecompileContext& sub_ctx) {
	SubDecompilePhaseCContext sub_c_ctx;
	for (
		list<shared_ptr<Block>>::const_iterator it_blk_seq = sub_ctx.blk_root->list_blk_child.cbegin();
		it_blk_seq != sub_ctx.blk_root->list_blk_child.cend();
		++it_blk_seq
		) {
		BlockSeqBlock* blk_seq = BlockSeqBlock::CastToMe(it_blk_seq->get());
		sub_c_ctx.map_it_blk_seq[blk_seq] = it_blk_seq;
	}
	if (!sub_ctx.blk_root->list_blk_child.empty()) {
		shared_ptr<BlockSeqBlock> blk_seq_first = dynamic_pointer_cast<BlockSeqBlock, Block>(sub_ctx.blk_root->list_blk_child.front());
		if (!blk_seq_first) throw(ErrDesignApp("Decompiler::DecompileSubPhaseC : cannot cast p to type \"BlockSeqBlock*\""));
		sub_c_ctx.set_ance.reserve(sub_ctx.blk_root->list_blk_child.size());
		SubDecompilePhaseCYield yield_child;
		this->DecompileSubPhaseCIterate(root_ctx, sub_ctx, sub_c_ctx, yield_child, blk_seq_first);
		// TODO: Process yield.
		// TODO: Process unreachable blocks.
	}
}

void Decompiler::DecompileSubPhaseCIterate(
	RootDecompileContext& root_ctx,
	SubDecompileContext& sub_ctx,
	SubDecompilePhaseCContext& sub_c_ctx,
	SubDecompilePhaseCYield& yield,
	shared_ptr<BlockSeqBlock> blk
) {
	switch (blk->exit_type) {
	case BlockSeqBlock::ExitType::Fallthrough:
		[[fallthrough]];
	case BlockSeqBlock::ExitType::Goto: {
		if (sub_c_ctx.set_ance.count(blk->vec_exit.at(0).blk_exit.get())) {
			// Yield reason: loop found.
			yield.yield_type = SubDecompilePhaseCYield::YieldType::LoopFound;
			yield.chain = shared_ptr<list<shared_ptr<BlockSeqBlock>>>(new list<shared_ptr<BlockSeqBlock>>());
			yield.chain->push_front(blk);
			return;
		}
		do {
			SubDecompilePhaseCYield yield_child;
			sub_c_ctx.set_ance.insert(blk.get());
			this->DecompileSubPhaseCIterate(root_ctx, sub_ctx, sub_c_ctx, yield_child, blk->vec_exit.at(0).blk_exit);
			sub_c_ctx.set_ance.erase(blk.get());
			if (!this->DecompileSubPhaseCProcessYield(root_ctx, sub_ctx, sub_c_ctx, yield, blk, yield_child)) break;
		} while (true);
		return;
	}
										// TODO: MultiExit.
	case BlockSeqBlock::ExitType::GotoIfZero: {
		if (sub_c_ctx.set_ance.count(blk->vec_exit.at(0).blk_exit.get()) || sub_c_ctx.set_ance.count(blk->vec_exit.at(1).blk_exit.get())) {
			// Yield reason: loop found.
			yield.yield_type = SubDecompilePhaseCYield::YieldType::LoopFound;
			yield.chain = shared_ptr<list<shared_ptr<BlockSeqBlock>>>(new list<shared_ptr<BlockSeqBlock>>());
			yield.chain->push_front(blk);
			return;
		}
		SubDecompilePhaseCYield yield_0;
		SubDecompilePhaseCYield yield_1;
		do {
			SubDecompilePhaseCYield yield_child;
			sub_c_ctx.set_ance.insert(blk.get());
			this->DecompileSubPhaseCIterate(root_ctx, sub_ctx, sub_c_ctx, yield_child, blk->vec_exit.at(0).blk_exit);
			sub_c_ctx.set_ance.erase(blk.get());
			if (!this->DecompileSubPhaseCProcessYield(root_ctx, sub_ctx, sub_c_ctx, yield_0, blk, yield_child)) break;
		} while (true);
		do {
			SubDecompilePhaseCYield yield_child;
			sub_c_ctx.set_ance.insert(blk.get());
			this->DecompileSubPhaseCIterate(root_ctx, sub_ctx, sub_c_ctx, yield_child, blk->vec_exit.at(0).blk_exit);
			sub_c_ctx.set_ance.erase(blk.get());
			if (!this->DecompileSubPhaseCProcessYield(root_ctx, sub_ctx, sub_c_ctx, yield_1, blk, yield_child)) break;
		} while (true);
		if (yield_0.yield_type == SubDecompilePhaseCYield::YieldType::ExitSubFound && yield_1.yield_type == SubDecompilePhaseCYield::YieldType::ExitSubFound) {
			yield.yield_type = SubDecompilePhaseCYield::YieldType::ExitSubFound;
			return;
		} else if (yield_0.yield_type == SubDecompilePhaseCYield::YieldType::LoopFound && yield_1.yield_type == SubDecompilePhaseCYield::YieldType::LoopFound) {
			yield.yield_type = SubDecompilePhaseCYield::YieldType::LoopFound;
			yield.chain = yield_0.chain->size() < yield_1.chain->size() ? yield_0.chain : yield_1.chain;
			return;
		} else if (yield_0.yield_type == SubDecompilePhaseCYield::YieldType::LoopFound || yield_1.yield_type == SubDecompilePhaseCYield::YieldType::LoopFound) {
			yield.yield_type = SubDecompilePhaseCYield::YieldType::LoopFound;
			yield.chain = yield_0.yield_type == SubDecompilePhaseCYield::YieldType::LoopFound ? yield_0.chain : yield_1.chain;
			return;
		} else {
			throw(ErrDesignApp("Decompiler::DecompileSubPhaseCIterate : unknown yield type combination"));
		}
		break;
	}
	case BlockSeqBlock::ExitType::GotoIfNonZero: {
		if (sub_c_ctx.set_ance.count(blk->vec_exit.at(0).blk_exit.get()) || sub_c_ctx.set_ance.count(blk->vec_exit.at(1).blk_exit.get())) {
			// Yield reason: loop found.
			yield.yield_type = SubDecompilePhaseCYield::YieldType::LoopFound;
			yield.chain = shared_ptr<list<shared_ptr<BlockSeqBlock>>>(new list<shared_ptr<BlockSeqBlock>>());
			yield.chain->push_front(blk);
			return;
		}
		SubDecompilePhaseCYield yield_0;
		SubDecompilePhaseCYield yield_1;
		do {
			SubDecompilePhaseCYield yield_child;
			sub_c_ctx.set_ance.insert(blk.get());
			this->DecompileSubPhaseCIterate(root_ctx, sub_ctx, sub_c_ctx, yield_child, blk->vec_exit.at(0).blk_exit);
			sub_c_ctx.set_ance.erase(blk.get());
			if (!this->DecompileSubPhaseCProcessYield(root_ctx, sub_ctx, sub_c_ctx, yield_0, blk, yield_child)) break;
		} while (true);
		do {
			SubDecompilePhaseCYield yield_child;
			sub_c_ctx.set_ance.insert(blk.get());
			this->DecompileSubPhaseCIterate(root_ctx, sub_ctx, sub_c_ctx, yield_child, blk->vec_exit.at(0).blk_exit);
			sub_c_ctx.set_ance.erase(blk.get());
			if (!this->DecompileSubPhaseCProcessYield(root_ctx, sub_ctx, sub_c_ctx, yield_1, blk, yield_child)) break;
		} while (true);
		if (yield_0.yield_type == SubDecompilePhaseCYield::YieldType::ExitSubFound && yield_1.yield_type == SubDecompilePhaseCYield::YieldType::ExitSubFound) {
			yield.yield_type = SubDecompilePhaseCYield::YieldType::ExitSubFound;
			return;
		} else if (yield_0.yield_type == SubDecompilePhaseCYield::YieldType::LoopFound && yield_1.yield_type == SubDecompilePhaseCYield::YieldType::LoopFound) {
			yield.yield_type = SubDecompilePhaseCYield::YieldType::LoopFound;
			yield.chain = yield_0.chain->size() < yield_1.chain->size() ? yield_0.chain : yield_1.chain;
			return;
		} else if (yield_0.yield_type == SubDecompilePhaseCYield::YieldType::LoopFound || yield_1.yield_type == SubDecompilePhaseCYield::YieldType::LoopFound) {
			yield.yield_type = SubDecompilePhaseCYield::YieldType::LoopFound;
			yield.chain = yield_0.yield_type == SubDecompilePhaseCYield::YieldType::LoopFound ? yield_0.chain : yield_1.chain;
			return;
		} else {
			throw(ErrDesignApp("Decompiler::DecompileSubPhaseCIterate : unknown yield type combination"));
		}
		break;
	}
	case BlockSeqBlock::ExitType::Return: {
		// Yield reason: subroutine exit found.
		yield.yield_type = SubDecompilePhaseCYield::YieldType::ExitSubFound;
		return;
	}
	case BlockSeqBlock::ExitType::ExitThread: {
		// Yield reason: subroutine exit found.
		yield.yield_type = SubDecompilePhaseCYield::YieldType::ExitSubFound;
		return;
	}
	default:
		throw(ErrDesignApp("Decompiler::DecompileSubPhaseCIterate : unknown exit type"));
	}
}

bool Decompiler::DecompileSubPhaseCProcessYield(
	DecompilerInternal::RootDecompileContext& root_ctx,
	DecompilerInternal::SubDecompileContext& sub_ctx,
	DecompilerInternal::SubDecompilePhaseCContext& sub_c_ctx,
	DecompilerInternal::SubDecompilePhaseCYield& yield,
	shared_ptr<DecompilerInternal::BlockSeqBlock> blk,
	DecompilerInternal::SubDecompilePhaseCYield& yield_child
) {
	switch (yield_child.yield_type) {
	case SubDecompilePhaseCYield::YieldType::ExitSubFound: {
		yield.yield_type = SubDecompilePhaseCYield::YieldType::ExitSubFound;
		yield.chain = yield_child.chain;
		yield.chain->push_front(blk);
		return false;
	}
	case SubDecompilePhaseCYield::YieldType::LoopFound: {
		/// <summary>The last block in the chain.</summary>
		shared_ptr<BlockSeqBlock> blk_end = yield_child.chain->back();
		switch (blk_end->exit_type) {
		case BlockSeqBlock::ExitType::Fallthrough:
			[[fallthrough]];
		case BlockSeqBlock::ExitType::Goto:
			if (blk_end->vec_exit.at(0).blk_exit == blk) {
				// TODO: Process loop found yield.
			} else {
				yield.yield_type = SubDecompilePhaseCYield::YieldType::LoopFound;
				yield.chain = yield_child.chain;
				yield.chain->push_front(blk);
				return false;
			}
			break;
			// TODO: MultiExit.
		case BlockSeqBlock::ExitType::GotoIfZero: {
			if (blk_end->vec_exit.at(0).blk_exit == blk || blk_end->vec_exit.at(1).blk_exit == blk) {
				// TODO: Process loop found yield.
			} else {
				yield.yield_type = SubDecompilePhaseCYield::YieldType::LoopFound;
				yield.chain = yield_child.chain;
				yield.chain->push_front(blk);
				return false;
			}
			break;
		}
		case BlockSeqBlock::ExitType::GotoIfNonZero: {
			if (blk_end->vec_exit.at(0).blk_exit == blk || blk_end->vec_exit.at(1).blk_exit == blk) {
				// TODO: Process loop found yield.
			} else {
				yield.yield_type = SubDecompilePhaseCYield::YieldType::LoopFound;
				yield.chain = yield_child.chain;
				yield.chain->push_front(blk);
				return false;
			}
			break;
		}
		default:
			throw(ErrDesignApp("Decompiler::DecompileSubPhaseCProcessYield : unknown exit type"));
		}
		break;
	}
	default:
		throw(ErrDesignApp("Decompiler::DecompileSubPhaseCProcessYield : unknown yield type"));
	}
}
