#!/bin/sh

for f in howdo/*.*.md; do cat "$f"; echo; echo "\\pagebreak"; done | pandoc -f markdown -o xdeya-howdo.pdf --pdf-engine=xelatex --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=howdo --lua-filter ../fix-links-single-file.lua

pandoc howdo/*.*.md -o xdeya-howdo.docx --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=howdo --reference-doc=../style-ref.docx --lua-filter ../fix-links-single-file.lua

#pandoc howdo/*.*.md -o xdeya-howdo.md --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=howdo --lua-filter fix-links-single-file.lua
