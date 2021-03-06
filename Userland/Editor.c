#include <stdio.h>
#include <readline/readline.h>
#include <fcntl.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

struct buffer {
    char *data;
    size_t size;
    size_t used;
    size_t nlines;
};

static void buffer_append_at_offset(struct buffer *buf, size_t offset, const char *data, size_t data_size)
{
    assert(buf->size >= buf->used + data_size);

    memmove(buf->data + offset + data_size, buf->data + offset, buf->used);
    memcpy(buf->data + offset, data, data_size);

    buf->used += data_size;
}

static int parse_integer(char *buffer, int *value_out, char **end_out)
{
    size_t offset = 0;
    int value = 0;

    while (isdigit(buffer[offset])) {
        value = value * 10 + (buffer[offset] - '0');
        ++offset;
    }

    if (offset == 0)
        return -1;

    if (value_out)
        *value_out = value;
    if (end_out)
        *end_out = buffer + offset;

    return 0;
}

static int buffer_get_line_offset(struct buffer *buf, size_t lineno, size_t *offset_out)
{
    assert(lineno >= 1);
    --lineno;

    size_t offset = 0;
    while (lineno > 0) {
        if (offset >= buf->used)
            return -1;

        if (buf->data[offset] == '\n') {
            --lineno;
            ++offset;
        } else {
            ++offset;
        }
    }

    if (offset_out)
        *offset_out = offset;

    return 0;
}

static void buffer_delete_range(struct buffer *buf, size_t from_line, size_t to_line)
{
    assert(from_line <= to_line);

    int retval;

    size_t from_offset;
    retval = buffer_get_line_offset(buf, from_line, &from_offset);
    assert(retval == 0);

    size_t to_offset;
    retval = buffer_get_line_offset(buf, to_line + 1, &to_offset);
    assert(retval == 0);

    assert(to_offset - from_offset <= buf->used);

    memmove(buf->data + from_offset, buf->data + to_offset, to_offset - from_offset);
    buf->used -= to_offset - from_offset;
    buf->nlines -= (to_line - from_line) + 1;
}

int main(int argc, char **argv) {
    if (argc > 2) {
        printf("ed: invalid operands\n");
        exit(1);
    }

    struct buffer buf = {
        .data = NULL,
        .size = 0,
        .used = 0,
        .nlines = 0,
    };

    ssize_t retval;

    const char *default_filename = NULL;

    if (argc == 2) {
        default_filename = argv[1];

        int fd = open(argv[1], O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        assert(fd >= 0);

        struct stat statbuf;
        retval = fstat(fd, &statbuf);
        assert(retval == 0);

        assert(statbuf.st_size != 0x0000dead);

        // FIXME: Make it possible for the buffer to grow
        buf.size = statbuf.st_size + 0x200;
        buf.data = malloc(buf.size);
        buf.used = statbuf.st_size;

        retval = read(fd, buf.data, statbuf.st_size);
        assert(retval == statbuf.st_size);
    } else {
        // FIXME: Make it possible for the buffer to grow
        buf.size = 0x200;
        buf.data = malloc(0x200);
    }

    for (;;) {
        char *raw_line = readline("% ");
        assert(raw_line != NULL);

        char *line = raw_line;

        int selection_start = -1;
        int selection_end = -1;

        if (parse_integer(line, &selection_start, &line) == 0) {
            if (*line == ',') {
                ++line;
                parse_integer(line, &selection_end, &line);
            }
        }

        if (*line == 'w') {
            ++line;

            assert(selection_start == -1);
            assert(selection_end == -1);

            if (*line == ' ') {
                ++line;

                default_filename = strdup(line);
            }

            int fd = open(
                default_filename,
                O_WRONLY | O_CREAT | O_TRUNC,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

            assert(fd >= 0);

            retval = write(fd, buf.data, buf.used);
            assert(retval >= 0);
            assert(retval == buf.used);

            goto next_iteration;
        } else if (*line == 'q') {
            ++line;

            assert(selection_start == -1);
            assert(selection_end == -1);
            assert(strlen(line) == 0);

            free(raw_line);
            break;
        } else if (*line == 'a') {
            ++line;

            assert(strlen(line) == 0);

            size_t offset = buf.used;

            if (selection_start != -1) {
                retval = buffer_get_line_offset(&buf, (size_t)selection_start + 1, &offset);
                assert(retval == 0);

                if (selection_end != -1)
                    buffer_delete_range(&buf, selection_start, selection_end);
            }

            for(;;) {
                char *new_line = readline("");

                assert(strlen(new_line) >= 1);

                if (strcmp(new_line, ".") == 0) {
                    free(new_line);
                    break;
                }

                buffer_append_at_offset(&buf, offset, new_line, strlen(new_line));
                offset += strlen(new_line);

                buffer_append_at_offset(&buf, offset, "\n", 1);
                offset += 1;
                buf.nlines += 1;

                free(new_line);
            }

            goto next_iteration;
        } else if (*line == 'p') {
            size_t start_offset = 0;
            size_t end_offset = buf.used;

            if (selection_start != -1) {
                retval = buffer_get_line_offset(&buf, selection_start, &start_offset);
                assert(retval == 0);

                retval = buffer_get_line_offset(&buf, selection_start + 1, &end_offset);
                assert(retval == 0);
            }
            if (selection_end != -1) {
                retval = buffer_get_line_offset(&buf, selection_end + 1, &end_offset);
                assert(retval == 0);
            }

            assert(end_offset >= start_offset);

            retval = write(STDOUT_FILENO, buf.data + start_offset, end_offset - start_offset);
            assert(retval == end_offset - start_offset);
        } else if (*line == 'l') {
            size_t start_line = 1;
            size_t end_line = buf.nlines + 1;

            if (selection_start != -1) {
                start_line = selection_start;
                end_line = selection_start + 1;
            }
            if (selection_end != -1)
                end_line = selection_end + 1;

            for (size_t lineno = start_line; lineno < end_line; ++lineno) {
                size_t start_offset;
                retval = buffer_get_line_offset(&buf, lineno, &start_offset);
                assert(retval == 0);

                size_t end_offset;
                retval = buffer_get_line_offset(&buf, lineno + 1, &end_offset);
                assert(retval == 0);

                printf("%zu ", lineno);

                retval = write(STDOUT_FILENO, buf.data + start_offset , end_offset - start_offset);
                assert(retval == end_offset - start_offset);
            }
        } else if (*line == 'd') {
            size_t start_line = 0;
            size_t end_line = buf.nlines + 1;

            if (selection_start != -1) {
                start_line = selection_start;
                end_line = selection_start;
            }
            if (selection_end != -1)
                end_line = selection_end;

            buffer_delete_range(&buf, start_line, end_line);
        } else {
            printf("ed: Unknown command\n");
        }

    next_iteration:
        free(raw_line);
    }

    free(buf.data);
}
