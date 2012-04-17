.PHONY: all clean docs text

all: setup.pdf simulation.pdf
docs: all

setup.pdf: setup.tex
	pdflatex setup.tex
	
simulation.pdf: simulation.tex
	pdflatex simulation.tex

text: setup.tex
	detex setup.tex > setup.txt

clean:
	rm -f setup.pdf simulation.pdf *.aux *.log setup.txt
