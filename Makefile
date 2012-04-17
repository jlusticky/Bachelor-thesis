# makefile pro preklad LaTeX verze Bc. prace
# (c) 2008 Michal Bidlo
# E-mail: bidlom AT fit vutbr cz
#===========================================
# asi budete chtit prejmenovat:
CO=projekt
DE=arbeit

all: $(CO).pdf $(DE).pdf

pdf: $(CO).pdf $(DE).pdf

$(CO).ps: $(CO).dvi
	dvips $(CO)

$(CO).pdf: clean
	pdflatex $(CO)
	bibtex $(CO)
	pdflatex $(CO)
	pdflatex $(CO)

$(DE).pdf: clean
	pdflatex $(DE)
	bibtex $(DE)
	pdflatex $(DE)
	pdflatex $(DE)

$(CO).dvi: $(CO).tex $(CO).bib
	latex $(CO)
	bibtex $(CO)
	latex $(CO)
	latex $(CO)

docs:
	make -C cd/docs/

text:
	make -C cd/docs/ text

desky:
#	latex desky
#	dvips desky
#	dvipdf desky
	pdflatex desky

clean:
	rm -f *.dvi *.log $(CO).blg $(CO).bbl $(CO).toc *.aux $(CO).out $(CO).lof
	rm -f $(CO).pdf
	rm -f *~
	make -C cd/docs/ clean
	./cleanup.sh yes
	rm -f $(DE).pdf

pack:
	tar czvf bp-xlusti00.tar.gz *.tex *.bib *.bst ./fig/* ./cls/* Makefile Changelog ./ntp/* ./contiki/* ./cd/*
