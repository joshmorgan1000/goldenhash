.PHONY: all clean view

MAIN = crocs_whitepaper
LATEX = pdflatex
BIBTEX = bibtex

all: $(MAIN).pdf

$(MAIN).pdf: $(MAIN).tex
	$(LATEX) $(MAIN)
	$(LATEX) $(MAIN)
	$(LATEX) $(MAIN)

bib: $(MAIN).tex references.bib
	$(LATEX) $(MAIN)
	$(BIBTEX) $(MAIN)
	$(LATEX) $(MAIN)
	$(LATEX) $(MAIN)

clean:
	rm -f *.aux *.log *.out *.toc *.bbl *.blg *.synctex.gz

view: $(MAIN).pdf
	open $(MAIN).pdf

help:
	@echo "Available targets:"
	@echo "  all   - Build the PDF"
	@echo "  bib   - Build with bibliography"
	@echo "  clean - Remove auxiliary files"
	@echo "  view  - Open the PDF"