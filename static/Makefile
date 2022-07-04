.PHONY: all clean

all: html-updater css/main.css

css/main.css: scss/main.scss
	mkdir -p css/
	sassc scss/main.scss css/main.css

html-updater:
	@../scripts/auto-update-html.sh

clean:
	$(RM) -r css/