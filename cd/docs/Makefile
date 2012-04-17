.PHONY: all clean

all: setup.pdf simulation.pdf

setup.pdf: setup.tex
	pdflatex setup.tex
	
simulation.pdf: simulation.tex
	pdflatex simulation.tex

clean:
	rm -f setup.pdf simulation.pdf *.aux *.log
