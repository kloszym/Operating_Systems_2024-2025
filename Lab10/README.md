# Laboratorium 10
## Zadanie 1: Problem lekarza
W ramach zadania należy zaimplementować rozwiązanie problemu synchronizacji w środowisku szpitalnym.

W małym szpitalu dyżuruje jeden lekarz, który obsługuje dwie grupy:

Pacjentów (np. 20 - ilosc pacjentów powinna byc konfigurowanlna), którzy przychodzą na konsultację,

Farmaceutów(np. 3 - ilosc farmaceutów powinna byc konfigurowanlna), którzy dostarczają leki do apteczki w szpitalu.

Lekarz jest człowiekiem bardzo zapracowanym i przemęczonym, więc kiedy idzie spać to śpi tak długo jak tylko może. Lekarz jak wspomniano jest zapracowany, więc przeprowadza konsultację tylko jeśli zbierze się dokładnie 3 pacjentów. Po konsultacji lekarz wraca do spania. Z powodu prawnie narzuconych restrykcji po pandemi w szpitalu może przebywać maksymalnie 3 pacjentów, a reszta nie jest w ogóle wpuszczana do szpitala.

Apteczka (leki) ma ograniczoną pojemność (np. 6), a każda konsultacja zużywa 3 jednostki leku (po jednym dla każdego pacjenta na konsultacji). Uzupełnianie apteczki jest możliwe dzięki farmaceutom.
Lekarz może przyjąć pacjentów tylko, jeśli ma dostępną wystarczającą ilość leków w apteczce (conajmniej 3).


#### Komunikaty 
Lekarz, pacjenci oraz farmaceuci powinni informować w formie komunikatów (informacji na standardowe wyjście o aktualnych etapach). Zmienne do komunikatów: TIME - dokładny czas w systemie, PATIENT_ID - identyfikator pacjenta, PHARMACIST_ID - identyfikator farmaceuty

### Zachowania pacjentów:
Przychodzą do szpitala po losowym czasie (2–5s).
(Komunikat: [TIME] - Pacjent(PATIENT_ID): Ide do szpitala, bede za _ s).

Jeśli w poczekalni czeka już 3 pacjentów – wracają później (robią sobie krótki spacer dookoła szpitala i znowu sprawdzają poczekalnie, jeśli dalej zajęta to znowu)
(Komunikat: [TIME] - Pacjent(PATIENT_ID): za dużo pacjentów, wracam później za _ s).

W przeciwnym razie siadaja w poczekalni
(Komunikat: [TIME] - Pacjent(PATIENT_ID): czeka _ pacjentów na lekarza).

Jeśli są trzecim pacjentem w poczekalni to budzą lekarza
(Komunikat: [TIME] - Pacjent(PATIENT_ID): budzę lekarza).

Po konsultacji wracają do domu
(Komunikat: [TIME] - Pacjent(PATIENT_ID): kończę wizytę).



### Zachowania farmaceutów:
Przybywają z nową dostawą po losowym czasie (5–15s).
(Komunikat: [TIME] - Farmaceuta(PHARMACIST_ID): ide do szpitala, bede za _ s).

Jeśli apteczka jest pełna – czekają na jej opróżnienie.
(Komunikat: [TIME] - Farmaceuta(PHARMACIST_ID): czekam na oproznienie apteczki).

Jeśli apteczka jest pusta lub się kończy (ilość leków < 3):

Budzą lekarza
(Komunikat: [TIME] - Farmaceuta(PHARMACIST_ID): budzę lekarza).

Dostarczają nową partię leków
(Komunikat: [TIME] - Farmaceuta(PHARMACIST_ID): dostarczam leki).

Po dostawie kończą pracę
(Komunikat: [TIME] - Farmaceuta(PHARMACIST_ID): zakończyłem dostawę).



### Zachowania lekarza:
Śpi, dopóki nie wystąpi jeden z warunków:

3 pacjentów czeka, i leki >= 3

farmaceuta czeka, i apteczka sie kończy leki < 3

Budzi się
(Komunikat: [TIME] - Lekarz: budzę się).

Jeśli są pacjenci i leki:

Konsultuje ich
(Komunikat: [TIME] - Lekarz: konsultuję pacjentów PATIENT_ID1, PATIENT_ID2, PATIENT_ID3),

Zużywa 3 leki,

Konsultacja trwa 2–4s.

W przeciwnym razie, jeśli są farmaceuci i miejsce w apteczce:

Odbiera dostawę
(Komunikat: [TIME] - Lekarz: przyjmuję dostawę leków),

Uzupełnia apteczke,

Dostawa trwa 1–3s.

Po wykonaniu działania wraca spać
(Komunikat: [TIME] - Lekarz: zasypiam.


Program należy zaimplementować korzystając z wątków (pthread) i mechanizmów synchronizacji biblioteki POSIX Threads (mutex, condition variable). Po uruchomieniu programu wątek główny tworzy wątki dla Lekarza, Pacjentów oraz Farmaceutów. Ilość pacjentów i farmaceutów powinna być możliwa do przekazania jako parametr uruchomieniowy programu (pierwszy parametr to ilość pacjentów, drugi parametr to ilość farmaceutów). Praca lekarza się kończy, gdy nie ma więcej pacjentów do wyleczenia. Do spania Lekarza powinny być wykorzystane Warunki Sprawdzające (Condition Variables). 

Użycie odpowiednich mechanizmów ma zagwarantować niedopouszczenie, np. do zdarzeń: Lekarz śpi chociaż czeka na niego 3 pacjentów lub Farmaceuta nie jest w stanie uzupełnić leków, więcej niż jeden farmaceuta uzupełnia apteczke itp.


#### 2 Wersje problemu:
Zadanie można oddać w 2 wariantach:

100 % - pełne rozwiązanie jak opisano powyżej (lekarz + pacjenci i farmaceuci)
60% - rozwiązanie częściowe (lekarz + pacjenci - bez farmaceutów i bez apteczki)
