.PHONY: all clean text

all: setup.pdf debugging.pdf

setup.pdf: setup.tex
	pdflatex setup.tex

debugging.pdf: debugging.tex
	pdflatex debugging.tex

text: setup.tex
	detex setup.tex > setup.txt

clean:
	rm -f setup.pdf debugging.pdf *.aux *.log *.out setup.txt
