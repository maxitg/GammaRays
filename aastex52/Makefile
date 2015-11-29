# Makefile for AASTeX

# Set INSTALLDIR to the directory that aastex.cls should be
# installed in.  It would be canonical for it to go in the
# tex/latex/misc area.  So you probably only need to change
# the leading portion of this, once you know where your texmf
# installation is rooted.

INSTALLDIR = /usr/share/texmf/tex/latex/misc

# It you have tetex installed, you can run "texconfig conf"
# to see what TEXMFMAIN is.

# ------------------------------------------------------------------------
# From here on, parameters should not usually require change.

PACKAGE = aastex
VERSION = 52
DATE    = 14 Jan 2004

TAREXT  = tgz

PERL    = /usr/bin/perl
FILEMDATE = /usr/local/bin/filemdate.pl
RM      = rm -f
TAR     = tar
COMPRESS = gzip
UNCOMPRESS = gunzip -c
INSTALL = install -m 0644
SYMLINK = ln -s

# ========================================================================
# NO!  You do not need to screw around below this line!

CLASS   = aastex.cls
MANUAL  = aasguide

SOURCES = *.tex *.eps $(CLASS) *.sty
SUBDIRS =
FILES   = Makefile Version README $(SOURCES) $(SUBDIRS)

# Add some useful inference rules for LaTeX development.
.SUFFIXES: .doc .sty .tex .dvi .ps .pdf

.doc.sty:
	doc2sty $*

.tex.pdf:
	latex $<
	latex $<
	dvips $< -o $*.ps
	ps2pdf $*.ps $*.pdf
	rm $*.ps

#.tex.dvi:
#	latex $<
#	latex $<

#.dvi.ps:
#	dvips $< -o $*.ps

#.dvi.pdf:
#	dvips $< -o $*.ps
#	ps2pdf $*.ps $*.pdf
#	rm $*.ps

# Generalized targets, housekeeping, etc.
first: showver help
all: $(MANUAL).pdf aassymbols.pdf natnotes.pdf sample.pdf table.pdf veryclean

install: $(INSTALLDIR)/$(CLASS)

$(INSTALLDIR)/$(CLASS): $(CLASS)
	$(INSTALL) $(CLASS) $(INSTALLDIR)

clean:
	$(RM) a.out core *.dvi *.log

veryclean: clean
	$(RM) *.aux *.idx *.toc

tar: clean Version
	$(RM) $(PACKAGE)$(VERSION).$(TAREXT)
	$(TAR) -cf - $(FILES) | $(COMPRESS) > $(PACKAGE)$(VERSION).$(TAREXT)

zip: clean
	$(RM) $(PACKAGE)$(VERSION).zip
	zip $(PACKAGE)$(VERSION).zip $(FILES)

Version: $(SOURCES)
	@echo -n "$(PACKAGE) v$(VERSION) ($(DATE)), " > $@
	@echo -n cls `egrep '%%[ ]+version[ ]+=' $(CLASS) | sed 's/^.*=[ ]*"//' | sed 's/".*$$//'` >> $@
	@echo -n ", last update " >> $@
	@$(FILEMDATE) `ls -tr $(SOURCES) | tail -1` >> $@
	@touch -r`ls -tr $(SOURCES) | tail -1` $@

showver: Version
	@cat Version
	@echo 'Version data in Makefile:'
	@egrep '^(PACKAGE|VERSION|DATE)' Makefile
	@echo -n 'Version data in $(CLASS): '
	@egrep '%%[ ]+version[ ]+=' $(CLASS) | sed 's/^.*=[ ]*"//' | sed 's/".*$$//'

help:
	@echo
	@echo To regenerate PDFs: make all
	@echo

texmf:
	@texconfig conf | egrep TEXMFMAIN

# Dependency targets.

$(MANUAL): $(MANUAL).dvi
$(MANUAL).pdf: $(MANUAL).tex
$(MANUAL).ps: $(MANUAL).dvi
$(MANUAL).dvi: $(MANUAL).tex $(CLASS)

aassymbols: aassymbols.dvi
aassymbols.pdf: aassymbols.tex
aassymbols.ps: aassymbols.dvi
aassymbols.dvi: aassymbols.tex $(CLASS)

sample: sample.dvi
sample.pdf: sample.tex
sample.ps: sample.dvi
sample.dvi: sample.tex $(CLASS)

natnotes: natnotes.dvi
natnotes.pdf: natnotes.tex
natnotes.ps: natnotes.dvi
natnotes.dvi: natnotes.tex

aasclass: aasclass.dvi
aasclass.pdf: aasclass.tex
aasclass.ps: aasclass.dvi
aasclass.dvi: aasclass.tex $(CLASS)
