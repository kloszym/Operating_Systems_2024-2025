# Laboratorium 2
1. Stwórz bibliotekę w języku C wystawiającą klientom następujące dwie funkcje:
  - int collatz_conjecture(int input) - funkcja realizująca regułę Collatza postaci:
```math
f(n) = \begin{cases}
\frac{n}{2} & \text{if } n \equiv 0 \pmod{2}, \\
3n + 1 & \text{if } n \equiv 1 \pmod{2}.
\end{cases}
```
Funkcja ta przyjmuje jedną liczbę typu całkowitego. Jeżeli jest ona parzysta, podziel ją przez 2 i zwróć wynik. Jeżeli jest nieparzysta, pomnóż ją przez 3 i dodaj 1, a następnie zwróć wynik.
  - int test_collatz_convergence(int input, int max_iter, int *steps) - funkcja sprawdzająca po ilu wywołaniach collatz_conjecture zbiega się do 1. Powinna ona wywoływać regułę Collatza najpierw na liczbie wejściowej a później na wyniku otrzymanym z reguły. W celu ochrony przed zbyt długim zapętlaniem się funkcji drugi parametr stanowi górną granicę liczby iteracji. Steps jest wyściowym argumentem zawieracjącym listę wynikową. Funkcja zwraca rozmiar tablicy steps czyli liczbę wykonanych kroków.
  - Zwróć do wywołującej funkcji sekwencję liczb prowadzącą do redukcji `input` do 1. W przypadku, gdy nie było to możliwe w `max_iter`, zwróć z funkcji 0.
2. W pliku makefile utwórz dwa wpisy: do kompilacji statycznej biblioteki i do kompilacji współdzielonej.
3. Napisz klienta korzystającego z kodu biblioteki, klient powinien sprawdzać kilka liczb, wykorzystując test_collatz_convergence, tj. po ilu iteracjach wynik zbiegnie się do 1 i wypisać
ałą sekwencję redukcji `input` do 1, gdy dało się to osiągnąć w `max_iter`. W przeciwnym wypadku wypisz komunikat o niepowodzeniu.
Klient powinien korzystać z biblioteki na 3 sposoby:
  - Jako biblioteka statyczna
  - Jako biblioteka współdzielona (linkowana dynamicznie)
  - Jako biblioteka ładowana dynamicznie
Dla każdego z wariantów utwórz odpowiedni wpis w Makefile. Do realizacji biblioteki dynamicznej użyj definicji stałej (-D) i dyrektywy preprocesora, aby zmodyfikować sposób działania klienta.
