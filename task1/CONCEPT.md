# ZADANIE 1: Śpiący fryzjerzy-kasjerzy

## Zachowanie procesów

### Proces-klient
* Zarabia pieniądze
* Część swojej wypłaty wkłada do portfela
* Idzie do fryzjera; staje w drzwiach ("blokując" je dla pozostałych wchodzących) aby zobaczyć, czy jest dla niego miejsce w poczekalni.
Jeśli tak, zajmuje je (wysyłając komunikat do kolejki), jednocześnie przekazując za ść portfela fryzjerowi; w przeciwnym wypadku wraca do pracy.
W obydwu przypadkach zwalnia drzwi, pozwalając kolejnej osobie wykonać te same czynności.
* Z punktu widzenia klienta, nie ma znaczenia moment rozpoczęcia usługi przez fryzjera
(gdyz jego stan nie zmienia się podczas jej rozpoczęcia, nie może np. w żadnym momencie przerwać czekania i zrezygnować).
* Po zakończeniu usługi, klient oczekuje na wydanie reszty przez fryzjera (który to musi najpierw dostać się do wspólnej kasy).
* W momencie otrzymania reszty, klient opuszcza salon i wykonuje tę pętlę po raz kolejny.

### Proces-fryzjer
* Oczekuje na pojawienie się klienta
* Odbiera od klienta zapłatę
* Oczekuje na wolny fotel w salonie


## Uzyte mechanizmy komunikacji i synchronizacji

### Poczekalnia
Poczekalnia jest zrealizowana z uzyciem kolejki komunikatów `waiting_room`. Zanim jednak proces-klient wyśle do niej komunikat informujący o swoim nadejściu,
sprawdza czy nie jest pełna, poprzez odczytanie liczby komunikatów które są w kolejce przechowywane (`msg_qnum`).
Aby zapobiec sytuacji w której dwóch klientów wchodzi w jednym momencie, powodując ze w poczekalni znajduje się `pojemność+1` klientów,
został zastosowany semafor `waiting_door`, strzegący drzwi poczekalni dla osób próbujących do niej wejść.

Komunikaty w `waiting_room` są typu `NEW_CUSTOMER` (wartość 1). Ich treść to 4 integery: 3 pierwsze to gotówka która jest przekazywana do fryzjera
(kolejno ilość monet o nominałach 1, 2 i 5) oraz PID klienta (żeby fryzjer wiedział kogo obsługuje).

### Wspólna kasa
Zawartość kasy jest reprezentowana przez segment pamięci współdzielonej pomiędzy procesami fryzjerów `cash_reg_id`. Ma ona trzy "przegródki",
w których znajdują się kolejno monety o nominałach 1, 2 i 5. Aby zapobiec problemom wywoływanym przez jednoczesny dostęp wielu procesów-fryzjerów do kasy,
strzeże jej semafor `cash_access`.

Kasa jest używana przez fryzjera dwukrotnie przy obsłudze każdego z klientów: przy rozpoczęciu usługi, fryzjer otrzymuje od niego pieniądze,
które umieszcza w kasie, a po jej zakończeniu wydaje z jej zawartości resztę, co jest realizowane prostym algorytmem zachłannym,
używanym przez większość kasjerek i kasjerów.
Wydawanie reszty może się ono nie udać w przypadku gdy w kasie nie ma monet o odpowiednim nominale (np. w kasie są tylko monety o nominale 5,
a należy wydać resztę równą 2). W takim wypadku próby wydania reszty są ponawiane bez skutku, w nadziei że pojawi się więcej drobnych.

### Fotele fryzjerskie

Wszystkie fotele są identyczne, więc każdy fryzjer/klient może zająć dowolny z nich i nie ma znaczenia który zostanie zajęty.
Fotele zostały więc zaimplementowane jako jeden semafor: `styling_chairs`, który na początku jest ustawiany na wartość równą ilości foteli w salonie.
Rozpoczynając pracę, fryzjer opuszcza go (o 1), a po jej zakończeniu podnosi (również o 1). Dzięki temu maksymalna liczba pracujących fryzjerów nie przekroczy
podanej liczby foteli, a każdy kolejny fryzjer przed rozpoczęciem pracy będzie czekał na zwolnienie któregokolwiek z nich.

### Informacja o zakończeniu pracy

Po wydaniu reszty, fryzjer wydaje resztę klientowi, informując go jednocześnie o zakończeniu pracy. W tym celu wysyła komunikat do kolejki `finished_q`.
Typ tego komunikatu to PID klienta, a jego zawartością jest zwrócona klientowi reszta (3 integery, kolejno ilość monet o nominałach 1, 2 i 5).

## Możliwe zakleszczenie

Istnieje szansa, że w kasie zabraknie monet o nominałach 1 i/lub 2, a nowi klienci nie będą się pojawiać (bo wciąż czekają na resztę lub na fryzjera
czekającego na resztę), przez co procesy fryzjerów nie będą w stanie wydać reszty i obsłużyć tych klientów, którzy być może przyszliby z drobnymi.
To zdarzenie jest zazwyczaj mało prawdopobodne, szanse rosną gdy liczba fryzjerów jest mała (np. 2).

## Busy waiting

Jeśli w kasie brakuje odpowiednich nominałów, procesy fryzjerów będą bez przerwy sprawdzać czy mogą już wydać resztę, nawet jeśli w kasie
nie pojawiły się nowe monety.

### Pomysł na rozwiązanie tego problemu z użyciem mechanizmów `pthread` (niezaimplementowany)

Po włożeniu pieniędzy od klienta, fryzjer wysyła sygnał `pthread_cond_signal` który zostanie odebrany przez jeden z procesów oczekujących na drobne
(lub zignorowany jeśli takich procesów nie ma). Proces który ten sygnał odbierze, spróbuje ponownie wydać resztę, po czym wyśle taki sam komunikat
(niezależnie czy próba była udana czy nie), aby pozostałe procesy również mogły ponowić tą próbę.
