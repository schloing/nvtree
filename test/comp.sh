set -e

gcc -g ../rb.c ../nvtree.c main.c -o ./a.out
rm -f ./trees/*
./a.out
echo "generating svgs from dots\nthis gunna take sum time. Jit"
for f in ./trees/*.dot; do
    dot -Tsvg "$f" -o "${f%.dot}.svg"
done