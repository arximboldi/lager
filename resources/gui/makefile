
all: gui.js gui.css

gui.js: gui.elm
ifeq ($(NIX_BUILD_TOP),)
	elm-make --yes --output $@ $<
else
	echo "calling elm-make with custom HOME:" $(TMP)
	HOME=$(TMP) elm-make --yes --output $@ $<
endif

gui.css: gui.scss
	sass --scss $< $@
