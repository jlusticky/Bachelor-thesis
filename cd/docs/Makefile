.PHONY: all clean text

all: setup.pdf simulation.pdf

setup.pdf: setup.tex
	pdflatex setup.tex

simulation.pdf: simulation.tex
	pdflatex simulation.tex

text: setup.tex
	detex setup.tex > setup.txt

clean:
	rm -f setup.pdf simulation.pdf *.aux *.log *.out setup.txt
