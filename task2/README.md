# Kompilacja

```
gcc water.c -Wall -pthread -o water.out
```

lub

```
./compile.sh
```

# Uruchamianie
```
./water.out H O
```

H: liczba wątków-producentów wodoru,

O: liczba wątków-producentów tlenu.

Jeśli te argumenty nie zostaną podane, zostaną wylosowane wartości: `2 <= H <= 10`, `1 <= O <= 10`.
Jeśli zostaną podane wartości `H < 2` lub `O < 1`, wykonywanie programu zakończy się błędem.
