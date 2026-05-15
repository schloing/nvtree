set -e

gcc -g rb.c nvtree.c -o ./a.out
./a.out
echo "generating svgs from dots\nthis gunna take sum time. Jit"
for f in ./test/trees/*.dot; do
    dot -Tsvg "$f" -o "${f%.dot}.svg"
done