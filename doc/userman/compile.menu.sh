#!/bin/sh

for f in menu/*.*.md; do cat "$f"; echo; echo "\\pagebreak"; done | pandoc -f markdown -o xdeya-menu.pdf --pdf-engine=xelatex --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=menu --lua-filter ../fix-links-single-file.lua

pandoc menu/*.*.md -o xdeya-menu.docx --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=menu --reference-doc=../style-ref.docx --lua-filter ../fix-links-single-file.lua

#pandoc menu/*.*.md -o xdeya-menu.md --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=menu --lua-filter fix-links-single-file.lua
