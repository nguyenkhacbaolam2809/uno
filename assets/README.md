# Assets

## Directory Structure

```
assets/
├── sounds/       # WAV/OGG sound effects
│   ├── card_slide.wav
│   ├── card_draw.wav
│   ├── uno.wav
│   ├── catch_uno.wav
│   ├── win.wav
│   ├── lose.wav
│   ├── hover.wav
│   ├── click.wav
│   ├── reverse.wav
│   ├── skip.wav
│   └── wild.wav
└── fonts/        # TTF font files (optional)
    └── main.ttf
```

## Sound Credits

Place sound effect files in `assets/sounds/`. The game gracefully
degrades if sounds are missing — no crashes, just silent operation.

## Fonts

If `assets/fonts/main.ttf` exists, it is used for UI text.
Otherwise, the default Raylib font is used.
