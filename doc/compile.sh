#!/bin/sh

pandoc userman/*.*.md -o userman.pdf --pdf-engine=xelatex --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=userman --lua-filter fix-links-single-file.lua

pandoc userman/*.*.md -o userman.docx --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=userman --reference-doc=style-ref.docx --lua-filter fix-links-single-file.lua

#pandoc userman/*.*.md -o userman.md --toc -N -M date="`date "+%e %B, %Y"`" --resource-path=userman --lua-filter fix-links-single-file.lua