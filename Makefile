CFLAGS += -I. -DHAVE_CONFIG_H `pkg-config --cflags poppler-glib gtk+-2.0`
LDFLAGS += `pkg-config --libs poppler-glib gtk+-2.0`

BIN = src/raktpdf
OBJS = src/main.o src/rakt-window.o

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(BIN)
