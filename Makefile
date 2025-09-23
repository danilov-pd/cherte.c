.PHONY: clean

WARNING_FLAGS= -Wall
OPT_FLAGS= -O2

example_plot: example_plot.c cherte.c olive.c stb_image_write.h
	$(CC) -o example_plot $(WARNING_FLAGS) $(OPT_FLAGS) example_plot.c -lm

olive.c:
	wget 'https://github.com/tsoding/olive.c/raw/2e20ec191cb92f65e3054a5f5e1eee2599ecb33c/olive.c'

stb_image_write.h:
	wget 'https://github.com/nothings/stb/raw/fede005abaf93d9d7f3a679d1999b2db341b360f/stb_image_write.h'

clean:
	$(RM) example_plot
