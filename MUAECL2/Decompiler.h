#pragma once
#include <iostream>
#include "Misc.h"
#include "RawEclDecoder.h"

using namespace std;

namespace DecompilerInternal {
	struct RootDecompileContext;
	struct SubDecompileContext;
	struct SubDecompilePhaseCContext;
	struct SubDecompilePhaseCYield;
	struct Block;
	struct BlockSeqBlock;
	struct SingleInsBlock;
}

/// <summary>An MUAECL Decompiler that converts decoded raw ECL to MUAECL code.</summary>
class Decompiler final {
public:
	Decompiler();
	~Decompiler();
	void DecompileRoot(const DecodedRoot& root, ostream& stream);
protected:
	static const set<uint16_t> set_ins_controlflow;
	void DecompileSub(DecompilerInternal::RootDecompileContext& root_ctx, const DecodedSub& sub, map<uint32_t, string>& map_var_name, ostream& stream_stmts);
	/// <summary>
	/// Phase A.
	/// In this phase, <c>BlockSeqBlock</c> blocks are splitted based on jump targets and instruction time.
	/// After this phase, children of the root <c>BlockSeqBlock</c> block will not have jump targets or delta-times.
	/// Also, instructions like "goto" and "return" will be at the end of them.
	/// </summary>
	void DecompileSubPhaseA(DecompilerInternal::RootDecompileContext& root_ctx, DecompilerInternal::SubDecompileContext& sub_ctx);
	/// <summary>
	/// Phase B.
	/// In this phase, exit types and exits are set for the blocks.
	/// Instructions like "goto" and "return" will be eliminated.
	/// </summary>
	void DecompileSubPhaseB(DecompilerInternal::RootDecompileContext& root_ctx, DecompilerInternal::SubDecompileContext& sub_ctx);
	void DecompileSubPostPhaseBCheck(DecompilerInternal::RootDecompileContext& root_ctx, DecompilerInternal::SubDecompileContext& sub_ctx);
	/// <summary>
	/// Phase C.
	/// TODO: What's done in this phase?
	/// </summary>
	void DecompileSubPhaseC(DecompilerInternal::RootDecompileContext& root_ctx, DecompilerInternal::SubDecompileContext& sub_ctx);
	void DecompileSubPhaseCIterate(
		DecompilerInternal::RootDecompileContext& root_ctx,
		DecompilerInternal::SubDecompileContext& sub_ctx,
		DecompilerInternal::SubDecompilePhaseCContext& sub_c_ctx,
		DecompilerInternal::SubDecompilePhaseCYield& yield,
		shared_ptr<DecompilerInternal::BlockSeqBlock> blk
	);
	/// <returns>Whether the caller should iterate its exit(s) again.</returns>
	bool DecompileSubPhaseCProcessYield(
		DecompilerInternal::RootDecompileContext& root_ctx,
		DecompilerInternal::SubDecompileContext& sub_ctx,
		DecompilerInternal::SubDecompilePhaseCContext& sub_c_ctx,
		DecompilerInternal::SubDecompilePhaseCYield& yield,
		shared_ptr<DecompilerInternal::BlockSeqBlock> blk,
		DecompilerInternal::SubDecompilePhaseCYield& yield_child
	);
};
