local M = {}

function M.setup()
  local syntax_file = vim.fn.expand("$HOME/.config/nvim/syntax/orta.vim")
  local content = [[
if exists("b:current_syntax")
  finish
endif

syntax case ignore

syntax match asmComment ";.*$"

syntax keyword asmRegister rax rbx rcx rdx rsi rdi r8 r9 fr

syntax keyword asmInstruction push mov pop add sub mul div mod
syntax keyword asmInstruction and or xor not eq ne lt gt le ge
syntax keyword asmInstruction jmp jmpif call ret load store
syntax keyword asmInstruction print dup swap drop rotl rotr
syntax keyword asmInstruction alloc halt merge xcall write
syntax keyword asmInstruction printmem sizeof dec inc

syntax match asmNumber "\<\d\+\>"
syntax match asmHexNumber "\<0x[0-9a-fA-F]\+\>"

syntax match asmLabel "^\s*\zs[a-zA-Z0-9_]\+:"
syntax match asmLabelRef "\<[a-zA-Z0-9_]\+\>"

syntax region asmString start=/"/ skip=/\\"/ end=/"/
syntax region asmString start=/'/ skip=/\\'/ end=/'/

highlight default link asmComment Comment
highlight default link asmRegister Identifier
highlight default link asmInstruction Statement
highlight default link asmNumber Number
highlight default link asmHexNumber Number
highlight default link asmLabel Function
highlight default link asmLabelRef Special
highlight default link asmString String

let b:current_syntax = "orta"
]]

  local file = io.open(syntax_file, "w")
  if file then
    file:write(content)
    file:close()
  end

  vim.api.nvim_create_autocmd({"BufNewFile", "BufRead"}, {
    pattern = {"*.x", "*.xh"},
    callback = function()
      vim.cmd("set filetype=orta")
    end
  })
end

return M
