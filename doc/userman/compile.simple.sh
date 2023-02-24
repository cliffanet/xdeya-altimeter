#!/bin/sh

pandoc simple.md -f markdown -o xdeya-simple.pdf --pdf-engine=xelatex --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=simple --lua-filter ../fix-links-single-file.lua

pandoc simple.md -o xdeya-simple.docx --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=simple --reference-doc=../style-ref.docx --lua-filter ../fix-links-single-file.lua

#pandoc simple.md -o xdeya-simple.md --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=simple --lua-filter fix-links-single-file.lua
