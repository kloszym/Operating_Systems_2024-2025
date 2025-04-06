# Laboratorium 1

Napisz prosty program countdown.c, który będzie w pętli odliczał od 10 do 0 i wypisywał aktualną liczbę na konsolę (każda liczba w nowej linii).
Stwórz plik Makefile, za pomocą którego skompilujesz swój program.
Makefile powinien zawierać co najmniej trzy targety: all, countdown, clean.

- all — powinien budować wszystkie targety (na razie tylko countdown, w kolejnych zadaniach będziemy dodawać nowe targety).
- countdown — buduje program countdown.c
- clean — usuwa wszystkie pliki binarne, czyści stan projektu

Użyj zmiennych CC oraz CFLAGS do zdefiniowania kompilatora (gcc) i flag compilacji (-Wall, -std=c17, ...). 
Dodatkowo w Makefile powinien być specjalny target .PHONY.
