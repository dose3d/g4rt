# Lista zadań G4RT - TODO

## 🔧 Konfiguracja i Setup

- **TLD.hh:17** - Rozwiązać konflikt między ROOT a CADMesh (problem z makrami i kolejnością includów)
- **ConfigSvc.hh:18** - Dodać elastyczność std::ostream

## 🎯 Akcje i Kontrola

- **ControlPoint.hh:33** - Wprowadzić FieldType jako enum (definicja w Types.hh)
- **RunAction.cc:71** - Przejrzeć i zweryfikować kod

## 📊 Analiza Danych

- **BeamAnalysis.cc:126** - Sprawdzić czy to samo co preStepPoint->GetTotalEnergy()
- **RunAnalysis.cc:21** - Zaimplementować RUN_CSV_ANALYSIS
- **RunAnalysis.cc:23** - Zaimplementować RUN_NTUPLE_ANALYSIS  
- **RunAnalysis.cc:25** - Zaimplementować RUN_HDF5_ANALYSIS
- **StepAnalysis.cc:63** - Zdefiniować podstawowe histogramy

## 🏗️ Geometria

### Linac

- **MlcHD120.cc:40** - Uzupełnić implementację
- **MlcSimplified.cc:20** - Pobrać wartości z konfiguracji
- **README.md:2** - Dodać tabelę z parametryzacją każdego modelu
- **BeamCollimation.cc:83** - Uzupełnić implementację

### Patient/Detektor

- **D3DCell.cc:157** - Przekazać pv i wydobyć współrzędne globalne
- **D3DCell.cc:182** - Wydobyć z zakresu Detector::name
- **D3DDetector.cc:26** - Uzupełnić implementację
- **D3DDetector.cc:209** - Zaimplementować metodę

### Phantom

- **DishCubePhantom.cc** - Zaimplementować metody (linie 24, 80, 99, 103, 107)
- **IbaImRT.cc:59** - Filtrowanie elementów do usunięcia z IbaImRT
- **IbaImRT.cc:60** - Stworzenie modułu do pobierania ścieżki do DB i CSV z PhantomWorld
- **SciSlicePhantom.cc:21,64** - Zaimplementować metody
- **WaterPhantom.cc:144** - Zaimplementować metodę
- **WaterPhantom.cc:152** - Zweryfikować implementację
- **PatientTest.hh:34** - Zmienić na std::unique
- **VPatientSD.hh:53** - Sprawdzić czy potrzebne

### VoxelHit

- **VoxelHit.cc:184** - Przechowywać wszystkie cząstki i interakcje (nie tylko elektrony)
- **VoxelHit.cc:209** - Przechowywać Parent ID lub Primary ID dla wizualizacji
- **VoxelHit.cc:241,257** - Obsłużyć błędy
- **VoxelHit.cc:268** - Zastąpić running 1/2-average prawdziwą średnią arytmetyczną
- **VoxelHit.cc:506** - Sprawdzić wzór na dawkę
- **VoxelHit_README.md:151** - Kontynuować dokumentację

### PhaseSpace

- **SavePhSpAnalysis.cc:54** - Przenieść kod z SavePhSpSD::ProcessHits
- **SavePhSpSD.cc:30** - Właściwe obsługiwanie zapisywania danych do katalogu phsp w NTuple

### WorldConstruction

- **LinacGeometry.cc:154** - Refaktoryzacja do smart pointers
- **LinacGeometry.hh:36** - Brak rotacji geometrii - cząstki powinny być rotowane po przejściu przez Jaws i MLC
- **PatientGeometry.cc:323** - Zaktualizować GetPhysicalVolume()
- **PatientGeometry.cc:439,539** - Uczynić generycznym dla każdego pacjenta
- **SavePhSpConstruction.cc:70** - Przenieść definicję do finalnego modelu
- **SavePhSpConstruction.cc:72** - Przejrzeć logikę tworzenia instancji SavePhSpSD
- **WorldConstruction.cc:340,348** - Uruchomić dla całego drzewa geometrii

## ⚛️ Fizyka

- **IaeaPrimaryGenerator.cc:27** - Sprawdzić funkcje
- **IaeaPrimaryGenerator.cc:32** - Sprawdzić konfigurację wielu plików PHSP
- **IonPrimaryGenerator.cc:27** - Sfinalizować specyfikację źródła
- **IonPrimaryGenerator.cc:33** - Skonfigurować ustawienia
- **PhysicsList.cc:14** - Rozważyć migrację do w pełni modularnej listy fizyki

## 🛠️ Usługi

- **DicomSvc.cc:65** - Refaktoryzacja kodu specyficznego dla kontekstu
- **DicomSvc.cc:138,147,380** - Uzupełnić implementację
- **DicomSvc.cc:384** - Zaimplementować FieldType::RTPlan
- **DicomSvc.cc:397** - Zaimplementować FieldType::CustomPlan
- **DicomSvc.hh:63** - Zastosować zasadę DRY
- **GeoSvc.cc:244** - Refaktoryzacja tymczasowego kodu
- **GeoSvc.cc:369** - Użyć typów enum
- **RunSvc.cc:271** - Zdefiniować tryby operacji
- **RunSvc.cc:465** - Sprawdzić warunek (zawsze true)
- **RunSvc.cc:511** - Zaimplementować metody eksportu konkretnych wolumenów
- **RunSvc.cc:565** - Rozwiązać problem z błędem przy zamykaniu pliku phasespace
- **RunSvc.cc:600** - Uzupełnić implementację
- **RunSvc.hh:25,26** - Zaimplementować TpFractionCounter i DaqTimeCounter
- **Services.cc:243** - Notatka o RDF::MakeCsvDataFrame

## 🔧 Narzędzia

- **UIManager.cc:106** - Zaimplementować runSvc->GetCurrentRun()

## 📁 Dane i Konfiguracja

- **gpsCLinac_pre.mac:80** - Sprawdzić /gps/ene/emspec 0
- **basic_iba_job.toml:27** - Jeśli istnieje -> odczytać i załadować
- **basic_gps.toml:25** - Zdefiniować typ
