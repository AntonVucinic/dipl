SHELL = /bin/sh

.SUFFIXES:

diplomski_rad.pdf: diplomski_rad.tex literatura.bib fer.cls hr_0036523672_94.pdf fer_IEEEtran_HR.bst
	latexmk -pdf $<

diplomski_rad.dvi: diplomski_rad.tex literatura.bib fer.cls hr_0036523672_94.pdf fer_IEEEtran_HR.bst
	latexmk $<

diplomski_rad.ps: diplomski_rad.tex literatura.bib fer.cls hr_0036523672_94.pdf fer_IEEEtran_HR.bst
	latexmk -ps $<

clean:
	latexmk -C diplomski_rad
	rm diplomski_rad.bbl
