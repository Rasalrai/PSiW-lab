# Zadanie 2: Cząsteczki wody

## Producenci

Każdy wątek-producent ma przypisany typ atomu, który produkuje: wodór lub tlen. Wykonuje on w nieskończonej pętli produkcję atomów
oraz następnie "składanie" ich w cząsteczki wody.

Produkcja atomu zabiera losową, niezerową i skończoną ilość czasu.

Łączenie atomów w cząsteczki wody jest sekcją krytyczną, więc może być wykonywane jednocześnie przez maksymalnie jeden wątek.

## Tworzenie cząsteczek

Po wytworzeniu atomu, producent wchodzi do sekcji krytycznej. __Aby opisać działanie bardziej obrazowo, skupię się na konkretnych działaniach producenta wodoru,
lecz producenci tlenu działają analogicznie.__

Producent informuje o wyprodukowaniu atomu, inkrementując licznik wolnych atomów wodoru. Następnie, podejmuje próbę utworzenia cząsteczki z dostępnych atomów
lub dołączenia swojego tworu do obecnie kompletowanej cząsteczki:

* jeśli do zbudowania obecnie tworzonych cząsteczek (lub cząsteczki) nie potrzeba więcej atomów wodoru, sprawdza czy jest w stanie rozpocząć tworzenie kolejnej cząsteczki:
czy liczba wolnych atomów jest do tego wystarczająca:
  * jeśli tak, "przenosi" 2 atomy wodoru i atom tlenu do licznika atomów potrzebnych, które wejdą w skład obecnie kompletowanych cząsteczek.
Wysyła więc sygnał do jednego producenta tlenu i jednego producenta wodoru, aby mogli dołączyć swoje atomy do cząsteczki którą on stworzył.
W tym momencie cząsteczkę uznaję za stworzoną, choć nie wiemy jeszcze które dokładnie atomy wejdą w jej skład. Przed opuszczeniem sekcji krytycznej
zaznacza użycie swojego atomu, dekrementując licznik potrzebnych atomów wodoru.
  * jeśli nie ma wystarczającej ilości wolnych atomów, zawiesza wykonywanie sekcji krytycznej, oczekując na sygnał od producenta który rozpocznie tworzenie
cząstki wody. Gdy wznowi jej wykonanie, upewnia się czy atom w jego buforze jest wciąż potrzebny: wtedy usuwa swój atom z atomów potrzebnych, odzwierciedlając
ten stan licznikiem. Jeśli nie, pewnie próbuje utworzyć nową cząsteczkę.

* jeśli potrzeba atomów wodoru, producent nie próbuje tworzyć nowej cząsteczki, lecz po prostu używa swoją aby uzupełnić inną, obecnie tworzoną.

Po zakończeniu wykonywania całej sekcji krytycznej, producent rozpoczyna cykl produkcji od nowa.

**Podsumowując** (TL;DR):
 * Gdy informacja o utworzeniu cząsteczki jest wysyłana, nie wiemy dokładnie które atomy wchodzą w jej skład: wiemy, że jej utworzenie
jest możliwe. Ponieważ wszystkie atomy danego pierwiastka są identyczne, nie ma znaczenia który konkretnie atom zostanie użyty.
 * Producent, który ma atom potrzebny w danej chwili i który wejdzie do sekcji krytycznej (lub powróci do niej po odebraniu sygnału),
dostaje informację, że jego atom został użyty w cząsteczce, więc może odjąć go od potrzebnych atomów i wyprodukować kolejny.
 * Producent, który po wejściu do sekcji krytycznej nie może dołączyć do istniejącej cząsteczki lub utworzyć nowej, zwalnia sekcję krytyczną,
 czekając na sygnał, że jego atom może być użyty.

**Priorytet zachowań w sekcji krytycznej**:

dołącz do istniejącej cząsteczki > utwórz nową cząsteczkę > czekaj na utworzenie cząsteczki przez kogoś innego


# Użyte mechanizmy synchronizacji

W programie jest jedna sekcja krytyczna, wspólna dla wszystkich producentów, więc potrzebny jest jeden mutex `create_water` strzegący do niej dostępu. Poza tym,
dwie zmienne warunkowe `h_wait` i `o_wait` pozwalają na znaczne zmniejszenie częstotliwości ponownych wejść wątków do sekcji krytycznej: wątek zostaje
uaktywniony tylko wtedy, gdy jest potrzebny atom, który on wyprodukował. Nie oznacza to, że zawsze po odebraniu tego sygnału będzie w stanie zakończyć
sekcję krytyczną (dlatego pętla `while (!h_needed)`, a nie warunek `if (!h_needed)`), gdyż być może wcześniej został wyprodukowany nowy atom potrzebnego
pierwiastka, który został użyty do budowy cząsteczki wody w pierwszym wejściu producenta do sekcji krytycznej.
