.PHONY: all clean docs

all: setup.pdf simulation.pdf
docs: all

setup.pdf: setup.tex
	pdflatex setup.tex
	
simulation.pdf: simulation.tex
	pdflatex simulation.tex

clean:
	rm -f setup.pdf simulation.pdf *.aux *.log
