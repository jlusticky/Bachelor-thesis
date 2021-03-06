# makefile pro preklad LaTeX verze Bc. prace
# (c) 2008 Michal Bidlo
# E-mail: bidlom AT fit vutbr cz
#===========================================
# asi budete chtit prejmenovat:
CO=projekt
DE=arbeit

all: clean $(CO).pdf $(DE).pdf

pdf: $(CO).pdf $(DE).pdf

$(CO).ps: $(CO).dvi
	dvips $(CO)

$(CO).pdf:
	pdflatex $(CO)
	bibtex $(CO)
	pdflatex $(CO)
	pdflatex $(CO)

$(DE).pdf:
	pdflatex $(DE)
	bibtex $(DE)
	pdflatex $(DE)
	pdflatex $(DE)

$(CO).dvi: $(CO).tex $(CO).bib
	latex $(CO)
	bibtex $(CO)
	latex $(CO)
	latex $(CO)

desky:
#	latex desky
#	dvips desky
#	dvipdf desky
	pdflatex desky

clean:
	rm -f *.dvi *.log $(CO).blg $(CO).bbl $(CO).toc *.aux $(CO).out $(CO).lof
	rm -f $(CO).pdf
	rm -f *~
	rm -f $(DE).acn $(DE).blg $(DE).bbl $(DE).toc $(DE).out $(DE).lof $(DE).glo $(DE).ist
	rm -f $(DE).pdf
	rm -f desky.pdf

pack:
	tar czvf bp-xlusti00.tar.gz *.tex *.bib *.bst ./fig/* Makefile hsrm-logo.eps hsrm-logo.pdf ./ntp/* ./contiki/* ./cd/*
