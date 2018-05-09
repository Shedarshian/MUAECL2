syn match   word "\h\w*"
syn region  Comment start="//" end="\n"
syn match   to "->" conceal cchar=→
syn keyword Id id
syn match   empty "\\e" conceal cchar=ε
syn keyword Key as_op type if elsif else while for goto
syn match   dot "\\\." conceal cchar=·
syn match   step "I\d\+"
syn match   step "S\d\+"
syn region  conc matchgroup=new start="%" end="%" concealends



syn match   arrow1 "---"
syn match   arrow1 "-\{2,}>"


hi word gui=italic guifg=yellow
hi Id gui=bold
hi Key guifg=red
hi arrow1 guifg=green
hi CursorLine guibg=black gui=underline
hi CursorColumn guibg=#3F3F3F
hi conc guifg=cyan gui=italic
