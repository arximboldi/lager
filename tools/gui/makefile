
all: gui.js gui.css

gui.js: gui.elm
	elm-make --yes --output $@ $<

gui.css: gui.scss
	sass --scss $< $@
