SAMPLES := adhoc audio dialog helloWorld internetBrowser intraFont map net OSK rectAndFonts saveAndLoad sfont shapes

all:
	@for dir in $(SAMPLES); do \
		echo "==> Building $$dir"; \
		$(MAKE) -C $$dir || exit 1; \
	done

clean:
	@for dir in $(SAMPLES); do \
		$(MAKE) -C $$dir clean; \
	done

.PHONY: all clean
