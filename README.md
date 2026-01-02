# 🎵 MP3 Player (C) — Windows

Конзолен MP3 player, написан на C. Проектът е качен така, че да може да се преглежда кодът (за преподавател) и да се компилира лесно на Windows (за колеги).

> Repository съдържа: `scr/` (source), `third_party/` (външни header-и), `config.txt` (настройки) и `MusicPlayer.exe` (готов билд).  

---

## 📁 Структура на проекта

- `scr/` — целият source code (C файлове) :contentReference[oaicite:1]{index=1}  
- `third_party/` — външни библиотеки/хедъри (vendor-нати в проекта) :contentReference[oaicite:2]{index=2}  
- `config.txt` — конфигурация / настройки :contentReference[oaicite:3]{index=3}  
- `MusicPlayer.exe` — готова компилирана версия за Windows (ако искаш само да пуснеш) :contentReference[oaicite:4]{index=4}  

---

## ⬇️ Как да изтеглиш проекта

### Вариант 1: ZIP (най-лесно)
1) Отвори repo-то  
2) Натисни **Code → Download ZIP** :contentReference[oaicite:5]{index=5}  
3) Разархивирай

### Вариант 2: Git clone (препоръчително)
1) Инсталирай Git for Windows :contentReference[oaicite:6]{index=6}  
2) В CMD/PowerShell:
```bat
git clone https://github.com/VikTD/Mp3-player.git
cd Mp3-player
