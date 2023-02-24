#!/bin/sh

for f in tech/*.md; do cat "$f"; echo; echo "\\pagebreak"; done | pandoc -f markdown -o xdeya-tech.pdf --pdf-engine=xelatex --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=tech --lua-filter ../fix-links-single-file.lua

pandoc tech/*.md -o xdeya-tech.docx --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=tech --reference-doc=../style-ref.docx --lua-filter ../fix-links-single-file.lua

#pandoc tech/*.md -o xdeya-tech.md --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=tech --lua-filter fix-links-single-file.lua
