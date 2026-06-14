clear
if ! gcc main.c -o main -Os; then
  exit -1
fi
strip main
if ! ./main; then
  exit -1
fi
