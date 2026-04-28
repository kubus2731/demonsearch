# demonsearch

Projekt demona systemowego napisanego w języku C, służącego do rekurencyjnego przeszukiwania systemu plików pod kątem zadanych fragmentów nazw.

## Opis projektu

Program po uruchomieniu przekształca się w demona i wykonuje cykliczne skanowanie drzewa katalogów. Praca jest zorganizowana w architekturze procesów: proces nadzorczy (Supervisor) zarządza procesami roboczymi (Workerami), gdzie każdy Worker odpowiada za wyszukiwanie pojedynczego wzorca nazwy. Program kładzie nacisk na poprawne wykorzystanie mechanizmów systemowych POSIX, takich jak obsługa sygnałów, zarządzanie procesami oraz logowanie systemowe.

## Kluczowe funkcjonalności

* Automatyczna demonizacja procesu (odłączenie od terminala, zmiana sesji, obsługa pliku PID).
* Współbieżne przeszukiwanie systemu plików przy użyciu wielu procesów potomnych.
* Rekurencyjne przeszukiwanie katalogów z uwzględnieniem uprawnień dostępu.
* Raportowanie wyników (pełna data, ścieżka, wzorzec) do logu systemowego syslog.
* Tryb wyczerpujący (verbose) logujący zdarzenia wewnętrzne demona.
* Zaawansowana obsługa sygnałów do sterowania działaniem programu bez jego restartu.

## Obsługa sygnałów

Demon reaguje na sygnały systemowe wysyłane do procesu nadzorczego:

* SIGUSR1: Jeśli demon śpi, zostaje natychmiast wybudzony do skanowania. Jeśli trwa skanowanie, zostaje ono zrestartowane od początku.
* SIGUSR2: Powoduje natychmiastowe przerwanie trwającego skanowania i przejście demona w tryb uśpienia.
* SIGINT / SIGTERM: Powoduje bezpieczne zakończenie pracy wszystkich procesów roboczych i zamknięcie demona.

## Wymagania i kompilacja

Projekt przeznaczony jest dla systemów operacyjnych z rodziny Linux/Unix. Do kompilacji wymagany jest standardowy kompilator GCC oraz narzędzie Make.

Kompilacja projektu:
make

## Użycie

Składnia uruchomienia:
./demonsearch [opcje] <wzorzec1> [wzorzec2 ...]

Dostępne opcje:
* -t, --time <sekundy> : Ustawienie interwału między skanami (domyślnie 60s).
* -d, --dir <ścieżka> : Katalog startowy wyszukiwania (domyślnie korzeń /).
* -v, --verbose : Włączenie szczegółowego logowania zdarzeń.
* -h, --help : Wyświetlenie pomocy.

Przykład uruchomienia:
./demonsearch -v -t 30 -d /home/user tajne dane

## Architektura kodu

Kod został podzielony na moduły odpowiedzialne za konkretne zadania:
* args: Parsowanie argumentów i konfiguracja runtime.
* daemonize: Procedury przekształcania procesu w demona.
* supervisor/worker: Zarządzanie procesami i orkiestracja zadań.
* scanner: Logika rekurencyjnego przeszukiwania drzewa plików.
* signals/sleep_control: Asynchroniczna obsługa zdarzeń i sterowanie czasem.
* logger: Interfejs zapisu danych do syslog.

## Autorzy

* Jakub Borkowski
* Jakub Matusiewicz
