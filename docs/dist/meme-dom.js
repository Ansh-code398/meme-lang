// MEMElang Reactive Web DOM Runtime (60-120 FPS Standard HTML/CSS/JS + Lazy Wasm Coprocessor)
export const __MEME_CFG__ = {
  title: "MEMElang Docs & Playground",
  strictPermissions: false,
  webPermissions: ["clipboard"]
};
const LUCIDE_PATHS = {
  "building": "M4 2H20V22H4Z M9 22V18H15V22 M8 6H8.01 M16 6H16.01 M12 6H12.01 M12 10H12.01 M12 14H12.01 M16 10H16.01 M16 14H16.01 M8 10H8.01 M8 14H8.01",
  "at-sign": "M8 12A4 4 0 1 1 16 12A4 4 0 1 1 8 12 M16 8V13A3 3 0 0 0 22 13V12A10 10 0 1 0 15.92 21.2",
  "archive": "M21 8V21H3V8 M1 3H23V8H1Z M10 12H14",
  "radio": "M4.93 19.07A10 10 0 0 1 4.93 4.93 M7.76 16.24A6 6 0 0 1 7.76 7.76 M16.24 7.76A6 6 0 0 1 16.24 16.24 M19.07 4.93A10 10 0 0 1 19.07 19.07 M10 12A2 2 0 1 1 14 12A2 2 0 1 1 10 12",
  "move": "M5 9L2 12L5 15 M9 5L12 2L15 5 M15 19L12 22L9 19 M19 9L22 12L19 15 M2 12H22 M12 2V22",
  "feather": "M20.24 12.24A6 6 0 0 0 11.76 3.76L5 10.5V19H13.5Z M16 8L2 22 M17.5 15H9",
  "flag": "M4 15S5 14 8 14S13 16 16 16S20 15 20 15V3S19 4 16 4S11 2 8 2S4 3 4 3Z M4 22V15",
  "target": "M2 12A10 10 0 1 1 22 12A10 10 0 1 1 2 12 M6 12A6 6 0 1 1 18 12A6 6 0 1 1 6 12 M10 12A2 2 0 1 1 14 12A2 2 0 1 1 10 12",
  "percent": "M19 5L5 19 M5 6.5A1.5 1.5 0 1 1 8 6.5A1.5 1.5 0 1 1 5 6.5 M16 17.5A1.5 1.5 0 1 1 19 17.5A1.5 1.5 0 1 1 16 17.5",
  "rss": "M4 11A9 9 0 0 1 13 20 M4 4A16 16 0 0 1 20 20 M4 19A1 1 0 1 1 6 19A1 1 0 1 1 4 19",
  "shopping-cart": "M8 21A1 1 0 1 1 10 21A1 1 0 1 1 8 21 M19 21A1 1 0 1 1 21 21A1 1 0 1 1 19 21 M1 1H5L7.68 14.39A2 2 0 0 0 9.64 16H19.4A2 2 0 0 0 21.35 14.52L23 6H6",
  "phone": "M22 16.92V19.92A2 2 0 0 1 19.82 21.92A19.79 19.79 0 0 1 11.19 18.85A19.5 19.5 0 0 1 5.19 12.85A19.79 19.79 0 0 1 2.12 4.18A2 2 0 0 1 4.11 2H7.11A2 2 0 0 1 9.11 3.72C9.24 4.68 9.47 5.62 9.81 6.53A2 2 0 0 1 9.36 8.64L8.09 9.91A16 16 0 0 0 14.09 15.91L15.36 14.64A2 2 0 0 1 17.47 14.19C18.38 14.53 19.32 14.76 20.28 14.89A2 2 0 0 1 22 16.92Z",
  "credit-card": "M1 4H23V20H1Z M1 10H23",
  "hash-tag": "M4 9H20 M4 15H20 M10 3L8 21 M16 3L14 21",
  "power": "M18.36 6.64A9 9 0 1 1 5.64 6.64 M12 2V12",
  "brackets": "M8 21H5V3H8 M16 3H19V21H16",
  "hexagon": "M21 16V8A2 2 0 0 0 20 6.27L13 2.27A2 2 0 0 0 11 2.27L4 6.27A2 2 0 0 0 3 8V16A2 2 0 0 0 4 17.73L11 21.73A2 2 0 0 0 13 21.73L20 17.73A2 2 0 0 0 21 16Z",
  "check-square": "M9 11L12 14L22 4 M21 12V19A2 2 0 0 1 19 21H5A2 2 0 0 1 3 19V5A2 2 0 0 1 5 3H16",
  "underline": "M6 3V10A6 6 0 0 0 18 10V3 M4 21H20",
  "anchor": "M12 22V8 M5 12H2A10 10 0 0 0 22 12H19 M9 5A3 3 0 1 1 15 5A3 3 0 1 1 9 5",
  "palette": "M12 2C6.5 2 2 6.5 2 12C2 17.5 6.5 22 12 22C12.92 22 13.65 21.25 13.65 20.35C13.65 19.93 13.5 19.53 13.23 19.22C12.97 18.92 12.82 18.54 12.82 18.14C12.82 17.24 13.57 16.5 14.47 16.5H16.5C19.54 16.5 22 14.04 22 11C22 6.03 17.52 2 12 2Z",
  "loader": "M12 2V6 M12 18V22 M4.93 4.93L7.76 7.76 M16.24 16.24L19.07 19.07 M2 12H6 M18 12H22 M4.93 19.07L7.76 16.24 M16.24 7.76L19.07 4.93",
  "briefcase": "M2 7H22V21H2Z M16 21V5A2 2 0 0 0 14 3H10A2 2 0 0 0 8 5V21",
  "pie-chart": "M21.21 15.89A10 10 0 1 1 8 2.83 M22 12A10 10 0 0 0 12 2V12Z",
  "printer": "M6 9V2H18V9 M6 18H4A2 2 0 0 1 2 16V11A2 2 0 0 1 4 9H20A2 2 0 0 1 22 11V16A2 2 0 0 1 20 18H18 M6 14H18V22H6Z",
  "minimize": "M8 3V6A2 2 0 0 1 6 8H3 M21 8H18A2 2 0 0 1 16 6V3 M3 16H6A2 2 0 0 1 8 18V21 M16 21V18A2 2 0 0 1 18 16H21",
  "command": "M18 3A3 3 0 0 0 15 6V18A3 3 0 1 0 18 15H6A3 3 0 1 0 9 18V6A3 3 0 1 0 6 9H18A3 3 0 0 0 18 3Z",
  "award": "M6 8A6 6 0 1 1 18 8A6 6 0 1 1 6 8 M8.21 13.89L7 23L12 20L17 23L15.79 13.88",
  "bar-chart": "M12 20V10 M18 20V4 M6 20V16",
  "align-center": "M18 10H6 M21 6H3 M21 14H3 M18 18H6",
  "activity": "M22 12H18L15 21L9 3L6 12H2",
  "calendar": "M3 4H21V22H3Z M16 2V6 M8 2V6 M3 10H21",
  "clock": "M12 2A10 10 0 1 1 12 22A10 10 0 1 1 12 2Z M12 6V12L16 14",
  "hash": "M4 9H20 M4 15H20 M10 3L8 21 M16 3L14 21",
  "thumbs-up": "M14 9V5A3 3 0 0 0 11 2L7 11V22H18.28A2 2 0 0 0 20.28 20.3L21.66 11.3A2 2 0 0 0 19.66 9Z M7 22H4A2 2 0 0 1 2 20V13A2 2 0 0 1 4 11H7",
  "align-right": "M21 10H7 M21 6H3 M21 14H3 M21 18H7",
  "star": "M12 2L15.09 8.26L22 9.27L17 14.14L18.18 21.02L12 17.77L5.82 21.02L7 14.14L2 9.27L8.91 8.26Z",
  "heart": "M20.84 4.61A5.5 5.5 0 0 0 13.06 4.61L12 5.67L10.94 4.61A5.5 5.5 0 0 0 3.16 12.39L4.22 13.45L12 21.23L19.78 13.45L20.84 12.39A5.5 5.5 0 0 0 20.84 4.61Z",
  "message-square": "M21 15A2 2 0 0 1 19 17H7L3 21V5A2 2 0 0 1 5 3H19A2 2 0 0 1 21 5Z",
  "mail": "M2 4H22V20H2Z M22 6L12 13L2 6",
  "log-out": "M9 21H5A2 2 0 0 1 3 19V5A2 2 0 0 1 5 3H9 M16 17L21 12L16 7 M21 12H9",
  "log-in": "M15 3H19A2 2 0 0 1 21 5V19A2 2 0 0 1 19 21H15 M10 17L15 12L10 7 M15 12H3",
  "gift": "M20 12V22H4V12 M2 7H22V12H2Z M12 22V7 M12 7H7.5A2.5 2.5 0 1 1 7.5 2C11 2 12 7 12 7Z M12 7H16.5A2.5 2.5 0 1 0 16.5 2C13 2 12 7 12 7Z",
  "user": "M20 21V19A4 4 0 0 0 16 15H8A4 4 0 0 0 4 19V21 M8 7A4 4 0 1 1 16 7A4 4 0 1 1 8 7",
  "settings": "M9 12A3 3 0 1 1 15 12A3 3 0 1 1 9 12 M12 1V4 M12 20V23 M4.22 4.22L6.34 6.34 M17.66 17.66L19.78 19.78 M1 12H4 M20 12H23 M4.22 19.78L6.34 17.66 M17.66 6.34L19.78 4.22",
  "sparkles": "M12 3L13.8 8.2L19 10L13.8 11.8L12 17L10.2 11.8L5 10L10.2 8.2Z",
  "shield-alert": "M12 22S20 18 20 12V5L12 2L4 5V12C4 18 12 22 12 22Z M12 8V12 M12 16H12.01",
  "help-circle": "M12 2A10 10 0 1 1 12 22A10 10 0 1 1 12 2Z M9.09 9A3 3 0 0 1 14.92 10C14.92 12 12 13 12 13 M12 17H12.01",
  "alert-circle": "M12 2A10 10 0 1 1 12 22A10 10 0 1 1 12 2Z M12 8V12 M12 16H12.01",
  "x": "M18 6L6 18 M6 6L18 18",
  "alert-triangle": "M10.29 3.86L1.82 18A2 2 0 0 0 3.53 21H20.47A2 2 0 0 0 22.18 18L13.71 3.86A2 2 0 0 0 10.29 3.86Z M12 9V13 M12 17H12.01",
  "map-pin": "M21 10C21 17 12 23 12 23S3 17 3 10A9 9 0 0 1 21 10Z M9 10A3 3 0 1 1 15 10A3 3 0 1 1 9 10",
  "book": "M4 19.5A2.5 2.5 0 0 1 6.5 17H20 M4 19.5V4.5A2.5 2.5 0 0 1 6.5 2H20V22H6.5A2.5 2.5 0 0 1 4 19.5Z",
  "bluetooth": "M6.5 6.5L17.5 17.5L12 23V1L17.5 6.5L6.5 17.5",
  "languages": "M5 8L11 14 M4 14L10 8L12 5 M2 5H14 M7 2H8 M22 22L17 12L12 22 M14 18H20",
  "aperture": "M2 12A10 10 0 1 1 22 12A10 10 0 1 1 2 12 M14.31 8L20.05 17.94 M9.69 8H21.17 M7.38 12L13.12 2.06 M9.69 16L3.95 6.06 M14.31 16H2.83 M16.62 12L10.88 21.94",
  "globe": "M12 2A10 10 0 1 1 12 22A10 10 0 1 1 12 2Z M2 12H22 M12 2A15.3 15.3 0 0 1 16 12A15.3 15.3 0 0 1 12 22A15.3 15.3 0 0 1 8 12A15.3 15.3 0 0 1 12 2Z",
  "eye": "M1 12S5 4 12 4S23 12 23 12S19 20 12 20S1 12 1 12Z M9 12A3 3 0 1 1 15 12A3 3 0 1 1 9 12",
  "key": "M21 2L19 4M11.39 11.61A5.5 5.5 0 1 1 3.61 19.39A5.5 5.5 0 0 1 11.39 11.61ZM11.5 11.5L15.5 7.5L18.5 10.5L22 7",
  "braces": "M8 3H7A2 2 0 0 0 5 5V10A2 2 0 0 1 3 12A2 2 0 0 1 5 14V19A2 2 0 0 0 7 21H8 M16 21H17A2 2 0 0 0 19 19V14A2 2 0 0 1 21 12A2 2 0 0 1 19 10V5A2 2 0 0 0 17 3H16",
  "unlock": "M3 11H21V22H3Z M7 11V7A5 5 0 0 1 16.9 6",
  "cpu": "M5 4H19A1 1 0 0 1 20 5V19A1 1 0 0 1 19 20H5A1 1 0 0 1 4 19V5A1 1 0 0 1 5 4Z M9 9H15V15H9Z M9 1V4 M15 1V4 M9 20V23 M15 20V23 M20 9H23 M20 14H23 M1 9H4 M1 14H4",
  "lock": "M3 11H21V22H3Z M7 11V7A5 5 0 0 1 17 7V11",
  "flame": "M8.5 14.5A2.5 2.5 0 0 0 11 12C11 10.62 10.5 9.67 9.5 8.5C8.5 7.33 8 6 8.5 4.5C11 6 14 8.5 14 13A2 2 0 0 0 18 13C18 10 16 8 16 8C19 10 20 13 20 15A8 8 0 0 1 4 15C4 12.5 5.5 10 7 9C7 10.5 7 13 8.5 14.5Z",
  "shield-check": "M12 22S20 18 20 12V5L12 2L4 5V12C4 18 12 22 12 22Z M9 12L11 14L15 10",
  "volume-x": "M11 5L6 9H2V15H6L11 19Z M22 9L16 15 M16 9L22 15",
  "menu": "M4 6H20 M4 12H20 M4 18H20",
  "volume-2": "M11 5L6 9H2V15H6L11 19Z M15.54 8.46A5 5 0 0 1 15.54 15.54 M19.07 4.93A10 10 0 0 1 19.07 19.07",
  "pause": "M6 4H10V20H6Z M14 4H18V20H14Z",
  "rocket": "M4.5 16.5C3 17.76 2.5 21.5 2.5 21.5S6.24 21 7.5 19.5A2.12 2.12 0 0 0 4.5 16.5Z M12 15L9 12A22 22 0 0 1 11 8C13.3 5 16.7 3 21.5 2.5C21 7.3 19 10.7 16 13A22 22 0 0 1 12 15Z",
  "shield-off": "M19.69 14C20 12.69 20 12 20 12V5L12 2L8.84 3.18 M4.73 4.73L4 5V12C4 18 12 22 12 22A18.33 18.33 0 0 0 17.34 17.34 M1 1L23 23",
  "sun": "M7 12A5 5 0 1 1 17 12A5 5 0 1 1 7 12 M12 1V3 M12 21V23 M4.22 4.22L5.64 5.64 M18.36 18.36L19.78 19.78 M1 12H3 M21 12H23 M4.22 19.78L5.64 18.36 M18.36 5.64L19.78 4.22",
  "wrench": "M14.7 6.3A1 1 0 0 0 14.7 7.7L16.3 9.3A1 1 0 0 0 17.7 9.3L21.47 5.53A6 6 0 0 1 13.53 13.47L6.62 20.38A2.12 2.12 0 0 1 3.62 17.38L10.53 10.47A6 6 0 0 1 18.47 2.53Z",
  "zap": "M13 2L3 14L12 14L11 22L21 10L12 10Z",
  "shield": "M12 22S20 18 20 12V5L12 2L4 5V12C4 18 12 22 12 22Z",
  "file-text": "M14 2H6A2 2 0 0 0 4 4V20A2 2 0 0 0 6 22H18A2 2 0 0 0 20 20V8Z M14 2V8H20 M16 13H8 M16 17H8 M10 9H8",
  "file": "M14 2H6A2 2 0 0 0 4 4V20A2 2 0 0 0 6 22H18A2 2 0 0 0 20 20V8Z M14 2V8H20",
  "bug": "M8 2L9.88 3.88 M14.12 3.88L16 2 M9 7.13V6A3 3 0 0 1 15 6V7.13 M12 20A7 7 0 0 1 5 13V11H19V13A7 7 0 0 1 12 20Z M12 20V11 M2 13H5 M19 13H22 M4 7L6.5 9.5 M20 7L17.5 9.5",
  "square": "M4 4H20V20H4Z",
  "git-branch": "M6 3V15 M15 6A3 3 0 1 1 21 6A3 3 0 1 1 15 6 M3 18A3 3 0 1 1 9 18A3 3 0 1 1 3 18 M18 9A9 9 0 0 1 9 18",
  "list": "M8 6H21 M8 12H21 M8 18H21 M3 6H3.01 M3 12H3.01 M3 18H3.01",
  "send": "M22 2L11 13 M22 2L15 22L11 13L2 9Z",
  "grid": "M3 3H10V10H3Z M14 3H21V10H14Z M14 14H21V21H14Z M3 14H10V21H3Z",
  "layers": "M12 2L2 7L12 12L22 7Z M2 17L12 22L22 17 M2 12L12 17L22 12",
  "paperclip": "M21.44 11.05L12.25 20.24A6 6 0 0 1 3.76 11.75L12.95 2.56A4 4 0 0 1 18.61 8.22L9.41 17.41A2 2 0 0 1 6.59 14.59L15.07 6.1",
  "type": "M4 7V4H20V7 M9 20H15 M12 4V20",
  "cloud": "M18 10H16.74A8 8 0 1 0 9 20H18A5 5 0 0 0 18 10Z",
  "bar-chart-2": "M18 20V10 M12 20V4 M6 20V14",
  "server": "M2 4H22V10H2Z M2 14H22V20H2Z M6 7H6.01 M6 17H6.01",
  "github": "M9 19C4 20.5 4 16.5 2 16 M15 22V18.13A3.37 3.37 0 0 0 14.06 15.52C17.2 15.17 20.5 13.98 20.5 8.52A5.44 5.44 0 0 0 19 4.77A5.07 5.07 0 0 0 18.91 1S17.73 0.65 15 2.48A13.38 13.38 0 0 0 9 2.48C6.27 0.65 5.09 1 5.09 1A5.07 5.07 0 0 0 5 4.77A5.44 5.44 0 0 0 3.5 8.55C3.5 13.97 6.8 15.16 9.94 15.55A3.37 3.37 0 0 0 9 18.13V22",
  "database": "M3 5A9 3 0 1 1 21 5A9 3 0 1 1 3 5 M21 12C21 13.66 17 15 12 15C7 15 3 13.66 3 12 M3 5V19C3 20.66 7 22 12 22C17 22 21 20.66 21 19V5",
  "music": "M9 18V5L21 3V16 M3 18A3 3 0 1 1 9 18A3 3 0 1 1 3 18 M15 16A3 3 0 1 1 21 16A3 3 0 1 1 15 16",
  "box": "M21 16V8A2 2 0 0 0 20 6.27L13 2.27A2 2 0 0 0 11 2.27L4 6.27A2 2 0 0 0 3 8V16A2 2 0 0 0 4 17.73L11 21.73A2 2 0 0 0 13 21.73L20 17.73A2 2 0 0 0 21 16Z M3.27 6.96L12 12.01L20.73 6.96 M12 22.08V12",
  "terminal": "M4 17L10 11L4 5 M12 19H20",
  "trash": "M3 6H21 M19 6V20A2 2 0 0 1 17 22H7A2 2 0 0 1 5 20V6 M8 6V4A2 2 0 0 1 10 2H14A2 2 0 0 1 16 4V6",
  "sliders": "M4 21V14 M4 10V3 M12 21V12 M12 8V3 M20 21V16 M20 12V3 M1 14H7 M9 8H15 M17 16H23",
  "arrow-up": "M12 19V5 M5 12L12 5L19 12",
  "filter": "M22 3H2L10 12.46V19L14 21V12.46Z",
  "bookmark": "M19 21L12 16L5 21V5A2 2 0 0 1 7 3H17A2 2 0 0 1 19 5Z",
  "share": "M4 12V20A2 2 0 0 0 6 22H18A2 2 0 0 0 20 20V12 M16 6L12 2L8 6 M12 2V15",
  "upload": "M21 15V19A2 2 0 0 1 19 21H5A2 2 0 0 1 3 19V15 M17 8L12 3L7 8 M12 3V15",
  "download": "M21 15V19A2 2 0 0 1 19 21H5A2 2 0 0 1 3 19V15 M7 10L12 15L17 10 M12 15V3",
  "edit": "M11 4H4A2 2 0 0 0 2 6V20A2 2 0 0 0 4 22H18A2 2 0 0 0 20 20V13 M18.5 2.5A2.12 2.12 0 0 1 21.5 5.5L12 15L8 16L9 12Z",
  "option": "M3 3H9L15 21H21 M14 3H21",
  "hammer": "M15 12L8.5 18.5A2.12 2.12 0 0 1 5.5 15.5L12 9 M17.64 15L22 10.64 M20.91 11.7L19.12 9.91 M14 7L17.5 3.5L20.5 6.5L17 10 M9 6L12 3L16 7L13 10",
  "search": "M11 4A7 7 0 1 1 11 18A7 7 0 1 1 11 4Z M21 21L16.65 16.65",
  "save": "M19 21H5A2 2 0 0 1 3 19V5A2 2 0 0 1 5 3H16L21 8V19A2 2 0 0 1 19 21Z M17 21V13H7V21 M7 3V8H15",
  "code-2": "M18 16L22 12L18 8 M6 8L2 12L6 16 M14.5 4L9.5 20",
  "git-commit": "M9 12A3 3 0 1 1 15 12A3 3 0 1 1 9 12 M3 12H9 M15 12H21",
  "bold": "M6 4H14A4 4 0 0 1 14 12H6Z M6 12H15A4 4 0 0 1 15 20H6Z",
  "plus-circle": "M12 2A10 10 0 1 1 12 22A10 10 0 1 1 12 2Z M12 8V16 M8 12H16",
  "compass": "M12 2A10 10 0 1 1 12 22A10 10 0 1 1 12 2Z M16.24 7.76L14.12 14.12L7.76 16.24L9.88 9.88Z",
  "bell": "M18 8A6 6 0 0 0 6 8C6 15 3 17 3 17H21S18 15 18 8Z M13.73 21A2 2 0 0 1 10.27 21",
  "book-open": "M2 3H8A4 4 0 0 1 12 7V21A3 3 0 0 0 9 18H2Z M22 3H16A4 4 0 0 0 12 7V21A3 3 0 0 1 15 18H22Z",
  "image": "M3 3H21V21H3Z M8.5 7A1.5 1.5 0 1 1 8.5 10A1.5 1.5 0 1 1 8.5 7Z M21 15L16 10L5 21",
  "file-code": "M14 2H6A2 2 0 0 0 4 4V20A2 2 0 0 0 6 22H18A2 2 0 0 0 20 20V8Z M14 2V8H20 M10 13L8 15L10 17 M14 13L16 15L14 17",
  "copy": "M8 4H18A2 2 0 0 1 20 6V16 M4 8H14A2 2 0 0 1 16 10V20A2 2 0 0 1 14 22H4A2 2 0 0 1 2 20V10A2 2 0 0 1 4 8Z",
  "check-circle": "M22 11.08V12A10 10 0 1 1 16.07 2.86 M22 4L12 14.01L9 11.01",
  "zoom-out": "M11 3A8 8 0 1 1 11 19A8 8 0 1 1 11 3Z M21 21L16.65 16.65 M8 11H14",
  "align-left": "M17 10H3 M21 6H3 M21 14H3 M17 18H3",
  "minus": "M5 12H19",
  "tablet": "M4 2H20V22H4Z M12 18H12.01",
  "circle": "M2 12A10 10 0 1 1 22 12A10 10 0 1 1 2 12",
  "chevrons-up-down": "M7 15L12 20L17 15 M7 9L12 4L17 9",
  "check": "M20 6L9 17L4 12",
  "smartphone": "M5 2H19V22H5Z M12 18H12.01",
  "package": "M16.5 9.4L7.5 4.21 M21 16V8A2 2 0 0 0 20 6.27L13 2.27A2 2 0 0 0 11 2.27L4 6.27A2 2 0 0 0 3 8V16A2 2 0 0 0 4 17.73L11 21.73A2 2 0 0 0 13 21.73L20 17.73A2 2 0 0 0 21 16Z M3.27 6.96L12 12.01L20.73 6.96 M12 22.08V12",
  "trash-2": "M3 6H21 M19 6V20A2 2 0 0 1 17 22H7A2 2 0 0 1 5 20V6 M8 6V4A2 2 0 0 1 10 2H14A2 2 0 0 1 16 4V6 M10 11V17 M14 11V17",
  "chevron-left": "M15 18L9 12L15 6",
  "arrow-left": "M19 12H5 M12 19L5 12L12 5",
  "refresh-cw": "M23 4V10H17 M1 20V14H7 M3.51 9A9 9 0 0 1 18.36 5.64L23 10 M1 14L5.64 18.36A9 9 0 0 0 20.49 15",
  "home": "M3 9L12 2L21 9V20A2 2 0 0 1 19 22H5A2 2 0 0 1 3 20Z M9 22V12H15V22",
  "crosshair": "M2 12A10 10 0 1 1 22 12A10 10 0 1 1 2 12 M22 12H18 M6 12H2 M12 6V2 M12 22V18",
  "users": "M17 21V19A4 4 0 0 0 13 15H5A4 4 0 0 0 1 19V21 M5 7A4 4 0 1 1 13 7A4 4 0 1 1 5 7 M23 21V19A4 4 0 0 0 20 15.13 M16 3.13A4 4 0 0 1 16 10.87",
  "more-vertical": "M12 12H12.01 M12 5H12.01 M12 19H12.01",
  "more-horizontal": "M12 12H12.01 M19 12H19.01 M5 12H5.01",
  "external-link": "M15 3H21V9 M10 14L21 3 M18 13V19A2 2 0 0 1 16 21H5A2 2 0 0 1 3 19V8A2 2 0 0 1 5 6H11",
  "sidebar": "M3 3H21V21H3Z M9 3V21",
  "arrow-up-right": "M7 17L17 7 M7 7H17V17",
  "trending-up": "M23 6L13.5 15.5L8.5 10.5L1 18 M17 6H23V12",
  "clipboard": "M16 4H18A2 2 0 0 1 20 6V20A2 2 0 0 1 18 22H6A2 2 0 0 1 4 20V6A2 2 0 0 1 6 4H8 M8 2H16V6H8Z",
  "inbox": "M22 12H16L14 15H10L8 12H2 M5.45 5.11L2 12V18A2 2 0 0 0 4 20H20A2 2 0 0 0 22 18V12L18.55 5.11A2 2 0 0 0 16.76 4H7.24A2 2 0 0 0 5.45 5.11Z",
  "triangle": "M10.29 3.86L1.82 18A2 2 0 0 0 3.53 21H20.47A2 2 0 0 0 22.18 18L13.71 3.86A2 2 0 0 0 10.29 3.86Z",
  "chevron-up": "M18 15L12 9L6 15",
  "rotate-ccw": "M1 4V10H7 M3.51 15A9 9 0 1 0 5.64 5.64L1 10",
  "arrow-down": "M12 5V19 M19 12L12 19L5 12",
  "wifi": "M5 12.55A11 11 0 0 1 19.08 12.55 M1.42 9A16 16 0 0 1 22.58 9 M8.53 16.11A6 6 0 0 1 15.48 16.11 M12 20H12.01",
  "chevron-right": "M9 18L15 12L9 6",
  "tag": "M20.59 13.41L13.42 20.58A2 2 0 0 1 10.59 20.58L2 12V2H12L20.59 10.59A2 2 0 0 1 20.59 13.41Z M7 7H7.01",
  "info": "M12 2A10 10 0 1 1 12 22A10 10 0 1 1 12 2Z M12 16V12 M12 8H12.01",
  "eye-off": "M17.94 17.94A10.07 10.07 0 0 1 12 20C5 20 1 12 1 12A18.45 18.45 0 0 1 6.06 6.06 M1 1L23 23",
  "mic": "M9 5A3 3 0 0 1 15 5V12A3 3 0 0 1 9 12Z M19 10V12A7 7 0 0 1 5 12V10 M12 19V23 M8 23H16",
  "italic": "M19 4H10 M14 20H5 M15 4L9 20",
  "maximize": "M8 3H5A2 2 0 0 0 3 5V8 M21 8V5A2 2 0 0 0 19 3H16 M16 21H19A2 2 0 0 0 21 19V16 M3 16V19A2 2 0 0 0 5 21H8",
  "folder": "M22 19A2 2 0 0 1 20 21H4A2 2 0 0 1 2 19V5A2 2 0 0 1 4 3H9L11 6H20A2 2 0 0 1 22 8Z",
  "plus": "M12 5V19 M5 12H19",
  "trending-down": "M23 18L13.5 8.5L8.5 13.5L1 6 M17 18H23V12",
  "battery": "M2 7H18V17H2Z M22 11V13",
  "corner-down-right": "M15 10L20 15L15 20 M4 4V11A4 4 0 0 0 8 15H20",
  "x-circle": "M12 2A10 10 0 1 1 12 22A10 10 0 1 1 12 2Z M15 9L9 15 M9 9L15 15",
  "dollar-sign": "M12 1V23 M17 5H9.5A3.5 3.5 0 0 0 9.5 12H14.5A3.5 3.5 0 0 1 14.5 19H6",
  "code": "M16 18L22 12L16 6 M8 6L2 12L8 18",
  "zoom-in": "M11 3A8 8 0 1 1 11 19A8 8 0 1 1 11 3Z M21 21L16.65 16.65 M11 8V14 M8 11H14",
  "layout": "M3 3H21V21H3Z M3 9H21 M9 21V9",
  "chevron-down": "M6 9L12 15L18 9",
  "moon": "M21 12.79A9 9 0 1 1 11.21 3A7 7 0 0 0 21 12.79Z",
  "laptop": "M4 4H20V16H4Z M2 18H22",
  "link": "M10 13A5 5 0 0 0 17.54 13.54L20.54 10.54A5 5 0 0 0 13.47 3.47L11.75 5.18 M14 11A5 5 0 0 0 6.46 10.46L3.46 13.46A5 5 0 0 0 10.53 20.53L12.24 18.82",
  "arrow-right": "M5 12H19 M12 5L19 12L12 19",
  "monitor": "M2 3H22V17H2Z M8 21H16 M12 17V21",
  "video": "M23 7L16 12L23 17Z M1 5H15A2 2 0 0 1 17 7V17A2 2 0 0 1 15 19H3A2 2 0 0 1 1 17Z",
  "play": "M6 3L20 12L6 21Z",
  "clipboard-check": "M16 4H18A2 2 0 0 1 20 6V20A2 2 0 0 1 18 22H6A2 2 0 0 1 4 20V6A2 2 0 0 1 6 4H8 M8 2H16V6H8Z M9 14L11 16L15 12",
  "hash-circle": "M12 2A10 10 0 1 1 12 22A10 10 0 1 1 12 2Z M8 10H16 M8 14H16 M10 8V16 M14 8V16",
  "camera": "M23 19A2 2 0 0 1 21 21H3A2 2 0 0 1 1 19V8A2 2 0 0 1 3 6H7L9 3H15L17 6H21A2 2 0 0 1 23 8Z M8 13A4 4 0 1 1 16 13A4 4 0 1 1 8 13"
};

let rootContainer = null;
let rootEntry = null;
let currentVTree = null;
let renderScheduled = false;
let isRendering = false;
let stateChangedDuringRender = false;
const hookStateMap = new Map();
const pathCounts = new Map();
const componentStack = [];
let activeHooks = [];
let hookCursor = 0;

function flattenChildren(arr, out = []) {
  for (const ch of arr) {
    if (Array.isArray(ch)) flattenChildren(ch, out);
    else if (ch !== null && ch !== undefined && ch !== false && ch !== true) out.push(ch);
  }
  return out;
}

function h(type, props, ...children) {
  return { __vnode: true, type, props: props || {}, children: flattenChildren(children) };
}

function Fragment(props) {
  return h("__frag__", null, ...(props.children || []));
}

function createPainter2d(canvas) {
  const ctx = canvas.getContext("2d");
  return {
    rect_filled(x, y, w, h, r = 0, col = "#ffffff", a = 1.0) {
      if (!ctx) return;
      ctx.save();
      ctx.globalAlpha = a ?? 1.0;
      ctx.fillStyle = col || "#ffffff";
      ctx.beginPath();
      if (ctx.roundRect && r > 0) ctx.roundRect(x, y, w, h, r);
      else ctx.rect(x, y, w, h);
      ctx.fill();
      ctx.restore();
    },
    rect_stroke(x, y, w, h, r = 0, lw = 1, col = "#ffffff", a = 1.0) {
      if (!ctx) return;
      ctx.save();
      ctx.globalAlpha = a ?? 1.0;
      ctx.strokeStyle = col || "#ffffff";
      ctx.lineWidth = Math.max(0.5, lw ?? 1);
      ctx.beginPath();
      if (ctx.roundRect && r > 0) ctx.roundRect(x, y, w, h, r);
      else ctx.rect(x, y, w, h);
      ctx.stroke();
      ctx.restore();
    },
    circle_filled(cx, cy, r = 4, col = "#ffffff", a = 1.0) {
      if (!ctx) return;
      ctx.save();
      ctx.globalAlpha = a ?? 1.0;
      ctx.fillStyle = col || "#ffffff";
      ctx.beginPath();
      ctx.arc(cx, cy, Math.max(0.5, r), 0, Math.PI * 2);
      ctx.fill();
      ctx.restore();
    },
    circle_stroke(cx, cy, r = 4, lw = 1, col = "#ffffff", a = 1.0) {
      if (!ctx) return;
      ctx.save();
      ctx.globalAlpha = a ?? 1.0;
      ctx.strokeStyle = col || "#ffffff";
      ctx.lineWidth = Math.max(0.5, lw ?? 1);
      ctx.beginPath();
      ctx.arc(cx, cy, Math.max(0.5, r), 0, Math.PI * 2);
      ctx.stroke();
      ctx.restore();
    },
    line(x1, y1, x2, y2, lw = 1, col = "#ffffff", a = 1.0) {
      if (!ctx) return;
      ctx.save();
      ctx.globalAlpha = a ?? 1.0;
      ctx.strokeStyle = col || "#ffffff";
      ctx.lineWidth = Math.max(0.5, lw ?? 1);
      ctx.beginPath();
      ctx.moveTo(x1, y1);
      ctx.lineTo(x2, y2);
      ctx.stroke();
      ctx.restore();
    },
    text(x, y, txt, size = 13, col = "#e4e4e7", weight = 400, isMono = false, a = 1.0) {
      if (!ctx || !txt) return;
      ctx.save();
      ctx.globalAlpha = a ?? 1.0;
      ctx.fillStyle = col || "#e4e4e7";
      const fam = isMono ? "'JetBrains Mono', monospace" : "'Inter', system-ui, sans-serif";
      ctx.font = `${weight || 400} ${Math.max(8, size || 13)}px ${fam}`;
      ctx.textBaseline = "top";
      ctx.fillText(String(txt), x, y);
      ctx.restore();
    }
  };
}

function paintCanvasDom(el, paintFn) {
  if (!el || typeof paintFn !== "function") return;
  const rect = el.getBoundingClientRect();
  const w = Math.max(300, Math.round(rect.width || el.clientWidth || 520));
  const h = Math.max(220, Math.round(rect.height || el.clientHeight || 320));
  const dpr = window.devicePixelRatio || 1;
  if (el.width !== Math.round(w * dpr) || el.height !== Math.round(h * dpr)) {
    el.width = Math.round(w * dpr);
    el.height = Math.round(h * dpr);
  }
  const ctx = el.getContext("2d");
  if (ctx) {
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, w, h);
  }
  try {
    paintFn(createPainter2d(el), { x: 0, y: 0, width: w, height: h });
  } catch (err) {
    console.warn("onPaint error:", err);
  }
}

function expandVNode(vnode) {
  if (vnode === null || vnode === undefined || typeof vnode === "boolean") return null;
  if (typeof vnode !== "object" || !vnode.__vnode) {
    return { kind: "text", text: String(vnode) };
  }
  if (vnode.type === "__frag__") {
    return { kind: "el", tag: "div", props: { style: "display:contents" }, children: vnode.children.map(expandVNode).filter(Boolean) };
  }
  if (typeof vnode.type === "function") {
    const fnName = vnode.type.name || "Comp";
    const keyPart = vnode.props?.key !== undefined && vnode.props?.key !== null ? ":" + String(vnode.props.key) : "";
    const basePath = (componentStack.length ? componentStack[componentStack.length - 1] + "/" : "") + fnName + keyPart;
    const count = pathCounts.get(basePath) || 0;
    pathCounts.set(basePath, count + 1);
    const fullPath = basePath + "#" + count;
    componentStack.push(fullPath);
    if (!hookStateMap.has(fullPath)) hookStateMap.set(fullPath, []);
    const prevHooks = activeHooks;
    const prevCursor = hookCursor;
    let rendered = null;
    for (let pass = 0; pass < 20; pass++) {
      activeHooks = hookStateMap.get(fullPath);
      hookCursor = 0;
      stateChangedDuringRender = false;
      const compProps = { ...(vnode.props || {}), children: vnode.children };
      rendered = vnode.type(compProps);
      if (!stateChangedDuringRender) break;
    }
    activeHooks = prevHooks;
    hookCursor = prevCursor;
    componentStack.pop();
    const out = expandVNode(rendered);
    if (out && vnode.props?.key !== undefined && out.props) {
      out.key = vnode.props.key;
    }
    return out;
  }
  const tag = String(vnode.type || "div");
  const props = { ...(vnode.props || {}) };
  const expandedChildren = [];
  for (const ch of vnode.children) {
    const ec = expandVNode(ch);
    if (ec) expandedChildren.push(ec);
  }
  return { kind: "el", tag, key: props.key ?? null, props, children: expandedChildren };
}

function applyProps(el, prevProps, nextProps) {
  const prev = prevProps || {};
  const next = nextProps || {};
  for (const k of Object.keys(prev)) {
    if (k in next || k === "key" || k === "children") continue;
    if (k === "className" || k === "class") {
      if (el instanceof SVGElement) el.removeAttribute("class");
      else el.className = "";
    }
    else if (k.startsWith("on")) {
      const evName = k === "onChange" ? "input" : k.slice(2).toLowerCase();
      if (el.__memeHandlers?.[evName]) el.removeEventListener(evName, el.__memeHandlers[evName]);
    } else if (k !== "onPaint" && k !== "value" && k !== "checked") {
      el.removeAttribute(k);
    }
  }
  if (!el.__memeHandlers) el.__memeHandlers = {};
  for (const [k, v] of Object.entries(next)) {
    if (k === "key" || k === "children" || v === undefined) continue;
    if (k === "className" || k === "class") {
      const clsStr = v ? String(v) : "";
      if (el instanceof SVGElement) {
        if (el.getAttribute("class") !== clsStr) el.setAttribute("class", clsStr);
      } else if (el.className !== clsStr) {
        el.className = clsStr;
      }
    } else if (k === "testId") {
      el.setAttribute("data-meme-testid", String(v));
    } else if (k === "value" && ("value" in el)) {
      const sv = v === null ? "" : String(v);
      if (el.value !== sv) el.value = sv;
    } else if (k === "checked" && ("checked" in el)) {
      el.checked = Boolean(v);
    } else if (k === "style") {
      if (typeof v === "string") el.setAttribute("style", v);
      else if (v && typeof v === "object") Object.assign(el.style, v);
    } else if (k === "onPaint" && typeof v === "function") {
      el.__memeOnPaint = v;
      requestAnimationFrame(() => paintCanvasDom(el, v));
    } else if (k.startsWith("on") && typeof v === "function") {
      const evName = (k === "onChange" && (el.tagName === "INPUT" || el.tagName === "TEXTAREA"))
        ? "input"
        : (k === "onChange" ? "change" : k.slice(2).toLowerCase());
      if (el.__memeHandlers[evName] !== v) {
        if (el.__memeHandlers[evName]) el.removeEventListener(evName, el.__memeHandlers[evName]);
        el.__memeHandlers[evName] = v;
        el.addEventListener(evName, v);
      }
      el.__memeRawProps = el.__memeRawProps || {};
      el.__memeRawProps[k] = v;
    } else if (typeof v === "boolean") {
      if (v) el.setAttribute(k, "");
      else el.removeAttribute(k);
    } else if (v !== null) {
      el.setAttribute(k, String(v));
    }
  }
}

function createDomNode(v) {
  if (!v) return document.createTextNode("");
  if (v.kind === "text") {
    const tn = document.createTextNode(v.text);
    v._dom = tn;
    return tn;
  }
  const svgTags = new Set(["svg", "path", "circle", "rect", "line", "polyline", "polygon"]);
  const el = svgTags.has(v.tag)
    ? document.createElementNS("http://www.w3.org/2000/svg", v.tag)
    : document.createElement(v.tag);
  applyProps(el, null, v.props);
  for (const ch of v.children) {
    el.appendChild(createDomNode(ch));
  }
  v._dom = el;
  return el;
}

function patchDom(parent, prevV, nextV, index = 0) {
  const existing = prevV?._dom || parent.childNodes[index];
  if (!prevV && nextV) {
    const nd = createDomNode(nextV);
    if (existing) parent.insertBefore(nd, existing);
    else parent.appendChild(nd);
    return;
  }
  if (prevV && !nextV) {
    if (existing && existing.parentNode === parent) parent.removeChild(existing);
    return;
  }
  if (prevV.kind === "text" && nextV.kind === "text") {
    if (prevV.text !== nextV.text) existing.nodeValue = nextV.text;
    nextV._dom = existing;
    return;
  }
  if (prevV.kind !== nextV.kind || prevV.tag !== nextV.tag || prevV.key !== nextV.key) {
    const nd = createDomNode(nextV);
    if (existing && existing.parentNode === parent) parent.replaceChild(nd, existing);
    else parent.appendChild(nd);
    return;
  }
  nextV._dom = existing;
  applyProps(existing, prevV.props, nextV.props);
  const maxLen = Math.max(prevV.children.length, nextV.children.length);
  for (let i = 0; i < maxLen; i++) {
    patchDom(existing, prevV.children[i] || null, nextV.children[i] || null, i);
  }
}

function getInitialData() {
  if (typeof window === "undefined") return null;
  if (window.__MEME_SSR_DATA__) return window.__MEME_SSR_DATA__;
  if (typeof document !== "undefined") {
    const el = document.getElementById("__MEME_SSR_DATA__");
    if (el && el.textContent) {
      try {
        window.__MEME_SSR_DATA__ = JSON.parse(el.textContent);
        return window.__MEME_SSR_DATA__;
      } catch (_) {}
    }
  }
  return null;
}

function renderNow() {
  if (!rootContainer || !rootEntry) return;
  const ssrData = getInitialData() || {};
  isRendering = true;
  renderScheduled = false;
  pathCounts.clear();
  componentStack.length = 0;
  const raw = typeof rootEntry === "function" ? h(rootEntry, ssrData) : rootEntry;
  const nextVTree = expandVNode(raw);
  isRendering = false;
  if (!currentVTree && rootContainer.firstChild) {
    rootContainer.innerHTML = "";
  }
  patchDom(rootContainer, currentVTree, nextVTree, 0);
  currentVTree = nextVTree;
}

function scheduleRender() {
  if (isRendering) {
    stateChangedDuringRender = true;
    return;
  }
  if (renderScheduled) return;
  renderScheduled = true;
  queueMicrotask(renderNow);
}

function useState(initVal) {
  const idx = hookCursor++;
  const hooks = activeHooks;
  if (hooks.length <= idx) {
    hooks[idx] = typeof initVal === "function" ? initVal() : initVal;
  }
  const setter = (valOrFn) => {
    const next = typeof valOrFn === "function" ? valOrFn(hooks[idx]) : valOrFn;
    if (!Object.is(hooks[idx], next)) {
      hooks[idx] = next;
      scheduleRender();
    }
  };
  return [hooks[idx], setter];
}

function useRef(initVal) {
  const idx = hookCursor++;
  if (activeHooks.length <= idx) activeHooks[idx] = { current: initVal };
  return activeHooks[idx];
}

function useEffect(fn, deps) {
  const idx = hookCursor++;
  const prev = activeHooks[idx];
  const changed = !prev || !deps || deps.some((d, i) => !Object.is(d, prev.deps?.[i]));
  activeHooks[idx] = { deps };
  if (changed) queueMicrotask(() => { try { fn(); } catch (_) {} });
}

function useMemo(fn, deps) {
  const idx = hookCursor++;
  const prev = activeHooks[idx];
  if (prev && deps && deps.every((d, i) => Object.is(d, prev.deps?.[i]))) return prev.val;
  const val = fn();
  activeHooks[idx] = { val, deps };
  return val;
}

function useRouter() {
  const parseQuery = () => {
    const q = {};
    if (typeof window !== "undefined") {
      const sp = new URLSearchParams(window.location.search || "");
      for (const [k, v] of sp.entries()) q[k] = v;
    }
    return q;
  };
  return {
    path: typeof window !== "undefined" ? window.location.pathname : "/",
    query: parseQuery(),
    navigate(nextPath) {
      if (typeof window !== "undefined" && nextPath && window.location.pathname !== nextPath) {
        window.history.pushState({}, "", nextPath);
        scheduleRender();
      }
    }
  };
}

if (typeof window !== "undefined") {
  window.addEventListener("popstate", () => scheduleRender());
  window.addEventListener("resize", () => scheduleRender());
}

const shadcn = {
  ScrollArea: (p) => h("div", { className: p.className || "overflow-y-auto", testId: p.testId }, ...(p.children || [])),
  Popover: (p) => h("div", { className: p.className ? ("z-50 " + p.className) : "z-50 rounded-xl border border-zinc-700 bg-zinc-900 p-2 shadow-2xl", testId: p.testId }, ...(p.children || [])),
  Dialog: (p) => (p.open === false ? null : h("div", { className: "fixed inset-0 z-50 flex items-center justify-center bg-zinc-950/75 p-4" }, h("div", { className: p.className || "rounded-2xl border border-zinc-700 bg-zinc-900 p-6 max-w-lg w-full shadow-2xl", testId: p.testId }, ...(p.children || [])))),
  Card: (p) => h("div", { className: p.className || "rounded-xl border border-zinc-800 bg-zinc-900 p-4", testId: p.testId }, ...(p.children || [])),
  CardHeader: (p) => h("div", { className: p.className || "pb-2" }, ...(p.children || [])),
  CardTitle: (p) => h("h3", { className: p.className || "text-lg font-bold text-white" }, ...(p.children || [])),
  CardContent: (p) => h("div", { className: p.className || "pt-2" }, ...(p.children || [])),
  Button: (p) => h("button", { ...p, className: p.className || "px-3 py-1.5 rounded-lg bg-amber-500 text-zinc-950 font-semibold text-xs" }, ...(p.children || [])),
  Badge: (p) => h("span", { className: p.className || "px-2 py-0.5 rounded bg-zinc-800 text-xs font-medium text-zinc-200" }, ...(p.children || [])),
  Input: (p) => h("input", p),
  Tabs: (p) => h("div", { className: p.className || "flex flex-row gap-2" }, ...(p.children || [])),
  Separator: (p) => h("div", { className: p.className || "w-full border-t border-zinc-800 my-2" }),
  Platform: (p) => {
    const wants = p.web || p.browser || (!p.ios && !p.android && !p.mac && !p.windows && !p.linux && !p.desktop && !p.native);
    return wants ? h(Fragment, null, ...(p.children || [])) : null;
  }
};

function normalizeIconKey(raw) {
  let k = String(raw || "sparkles")
    .replace(/([a-z0-9])([A-Z])/g, "$1-$2")
    .replace(/[_\s]+/g, "-")
    .toLowerCase();
  if (k === "close") return "x";
  if (k === "warn" || k === "warning") return "alert-triangle";
  if (k === "gear") return "settings";
  if (k === "doc" || k === "docs") return "book-open";
  if (k === "code2") return "code";
  return k;
}

function IconComp(props) {
  const name = normalizeIconKey(props.name || "sparkles");
  const sz = props.size || 16;
  const strokeW = props.strokeWidth || 2;
  const color = props.color || "currentColor";
  const pathData = LUCIDE_PATHS[name] || LUCIDE_PATHS["sparkles"] || "M12 3L14 9L20 12L14 15L12 21L10 15L4 12L10 9Z";
  const dList = pathData.split(/\s+(?=M)/).filter(Boolean);
  return h("svg", {
    xmlns: "http://www.w3.org/2000/svg",
    width: sz,
    height: sz,
    viewBox: "0 0 24 24",
    fill: "none",
    stroke: color,
    "stroke-width": strokeW,
    "stroke-linecap": "round",
    "stroke-linejoin": "round",
    className: props.className || "inline-block shrink-0"
  }, ...dList.map((d) => h("path", { d })));
}

const iconsMod = new Proxy({ Icon: IconComp }, {
  get(target, prop) {
    if (prop in target) return target[prop];
    const name = normalizeIconKey(prop);
    return (p) => IconComp({ ...(p || {}), name });
  }
});

function useSpring(target) {
  const tgt = Number(target || 0);
  const [val, setVal] = useState(tgt);
  const stateRef = useRef({ cur: tgt, vel: 0, tgt });
  stateRef.current.tgt = tgt;
  useEffect(() => {
    let raf = 0;
    const step = () => {
      const s = stateRef.current;
      const disp = s.cur - s.tgt;
      if (Math.abs(disp) < 0.005 && Math.abs(s.vel) < 0.01) {
        s.cur = s.tgt;
        s.vel = 0;
        setVal(s.cur);
        return;
      }
      const force = -220 * disp - 24 * s.vel;
      s.vel += force * 0.016;
      s.cur += s.vel * 0.016;
      setVal(s.cur);
      raf = requestAnimationFrame(step);
    };
    if (Math.abs(stateRef.current.cur - stateRef.current.tgt) > 0.005) {
      raf = requestAnimationFrame(step);
    }
    return () => { if (raf) cancelAnimationFrame(raf); };
  }, [tgt]);
  return val;
}

const motionTag = (tag) => (props) => {
  const { initial, animate, transition, children, ...rest } = props || {};
  const animCls = (initial && initial.y != null) ? "animate-slide-up" : "animate-fade-in";
  const cls = ["transition-all duration-200", animCls, rest.className || ""].filter(Boolean).join(" ");
  const st = (typeof rest.style === "object" && rest.style) ? { ...rest.style } : {};
  if (animate && typeof animate === "object") {
    if (animate.opacity != null) st.opacity = String(animate.opacity);
    if (animate.y != null) st.transform = `translateY(${animate.y}px)`;
  }
  return h(tag, { ...rest, style: Object.keys(st).length ? st : rest.style, className: cls }, ...(children || []));
};

let evalModulePromise = null;
async function lazyEval(code, stdinText = "") {
  if (!evalModulePromise) {
    evalModulePromise = import("./meme-eval.js").then((mod) => {
      const factory = mod.default || mod.createMemeEvalModule;
      return typeof factory === "function" ? factory() : mod;
    });
  }
  const wasm = await evalModulePromise;
  const rawJson = wasm.ccall("meme_eval_json", "string", ["string", "string"], [String(code || ""), String(stdinText || "")]);
  return JSON.parse(rawJson);
}

const native = {
  clipboard: {
    write(txt) {
      try { navigator.clipboard?.writeText(String(txt || "")); } catch (_) {}
      return true;
    },
    set(txt) { return this.write(txt); },
    read() { return ""; },
    get() { return ""; }
  },
  device: {
    screen() {
      return {
        width: typeof window !== "undefined" ? window.innerWidth : 1280,
        height: typeof window !== "undefined" ? window.innerHeight : 820,
        scale: typeof window !== "undefined" ? (window.devicePixelRatio || 1) : 1
      };
    }
  },
  haptics: { impact() { return true; }, selection() { return true; } },
  notifications: { send() { return true; } }
};

const platform = {
  os() { return "web"; },
  name() { return "web"; },
  isWeb() { return true; },
  isNative() { return false; },
  info() { return { os: "web", platform: "web", engine: "dom-es6" }; }
};

const permissions = {
  check(cap) {
    const c = String(cap || "").toLowerCase();
    const ok = !__MEME_CFG__.strictPermissions || __MEME_CFG__.webPermissions.includes(c);
    return { state: ok ? "granted" : "denied", granted: ok, capability: c };
  },
  async request(cap) {
    return this.check(cap);
  },
  canAsk(cap) { return this.check(cap).granted; }
};

const tijori = {
  rakh(k, v) { try { localStorage.setItem("meme_tijori_" + k, JSON.stringify(v)); } catch (_) {} return true; },
  nikal(k) { try { const v = localStorage.getItem("meme_tijori_" + k); return v ? JSON.parse(v) : null; } catch (_) { return null; } }
};

function mount(entry, title) {
  rootEntry = entry;
  if (title && typeof document !== "undefined") document.title = title;
  if (!rootContainer && typeof document !== "undefined") {
    rootContainer = document.getElementById("app");
    if (!rootContainer) {
      rootContainer = document.createElement("div");
      rootContainer.id = "app";
      document.body.appendChild(rootContainer);
    }
  }
  renderNow();
}

function installAgentBridge() {
  if (typeof window === "undefined") return;
  window.__MEME_AGENT__ = {
    snapshot() {
      const nodes = [];
      let nextId = 1;
      const walk = (el) => {
        if (!el || el.nodeType !== 1) return;
        const rect = el.getBoundingClientRect();
        const style = window.getComputedStyle(el);
        const visible = style.display !== "none" && style.visibility !== "hidden" && rect.width > 0 && rect.height > 0;
        const hasClick = Boolean(el.__memeHandlers?.click) || el.tagName === "BUTTON";
        const hasInput = el.tagName === "INPUT" || el.tagName === "TEXTAREA";
        let name = "";
        if (hasInput) name = el.value || el.getAttribute("placeholder") || "";
        else name = (el.innerText || el.textContent || "").trim();
        if (visible && (hasClick || hasInput || el.getAttribute("data-meme-testid") || name)) {
          const nid = nextId++;
          el.__memeAgentId = nid;
          nodes.push({
            id: nid,
            tag: el.tagName.toLowerCase(),
            testId: el.getAttribute("data-meme-testid") || "",
            accessibleName: name.slice(0, 240),
            hasClick,
            hasInput,
            x: rect.x,
            y: rect.y,
            width: rect.width,
            height: rect.height
          });
        }
        for (const ch of el.children) walk(ch);
      };
      walk(rootContainer || document.body);
      return {
        viewport: { width: window.innerWidth, height: window.innerHeight },
        renderer: "dom",
        nodes
      };
    },
    act(idOrTestId, action = "click", value = "") {
      this.snapshot();
      const target = [...(rootContainer || document.body).querySelectorAll("*")].find(
        (el) => el.__memeAgentId === Number(idOrTestId) || el.getAttribute("data-meme-testid") === String(idOrTestId)
      );
      if (!target) return 0;
      if (action === "click") {
        target.click();
        return 1;
      }
      if ((action === "type" || action === "change") && ("value" in target)) {
        target.value = String(value);
        target.dispatchEvent(new Event("input", { bubbles: true }));
        return 1;
      }
      return 0;
    }
  };
}
installAgentBridge();

export const $m = {
  h,
  Fragment,
  motionTag,
  ui: {
    mount,
    dikha: mount,
    show(t) { if (t && typeof document !== "undefined") document.title = t; renderNow(); },
    useState,
    useEffect,
    useRef,
    useMemo,
    useRouter,
    getInitialData,
    Router: (p) => h(Fragment, null, ...(p.children || [])),
    Route: (p) => h(Fragment, null, ...(p.children || [])),
    Link: (p) => h("a", { href: p.to || "/", className: p.className || "", onClick: (e) => { e.preventDefault(); useRouter().navigate(p.to || "/"); } }, ...(p.children || [])),
    Icon: IconComp,
    useSpring,
    registerFont: () => true
  },
  shadcn: { ...shadcn, Icon: IconComp },
  icons: iconsMod,
  motion: (() => {
    const mt = {
      div: motionTag("div"),
      span: motionTag("span"),
      button: motionTag("button"),
      section: motionTag("section"),
      article: motionTag("article"),
      aside: motionTag("aside"),
      header: motionTag("header"),
      footer: motionTag("footer"),
      p: motionTag("p"),
      h1: motionTag("h1"),
      h2: motionTag("h2"),
      h3: motionTag("h3"),
      ul: motionTag("ul"),
      li: motionTag("li"),
      View: motionTag("div"),
      useSpring,
      spring: useSpring
    };
    mt.motion = mt;
    return mt;
  })(),
  native,
  platform,
  permissions,
  tijori,
  meme: { eval: lazyEval, run: lazyEval, exec: lazyEval, chala: lazyEval, version: () => "MEMElang Web DOM 1.0" },
  eval: lazyEval,
  json: { parse: (s) => JSON.parse(s), stringify: (v) => JSON.stringify(v) },
  crypto: { sha256: async (s) => String(s), uuid_v4: () => (crypto?.randomUUID ? crypto.randomUUID() : "00000000-0000-4000-8000-000000000000") },
  bridgePkg: (name) => ({ __bridge: name }),
  size(x) {
    if (x === null || x === undefined) return 0;
    if (typeof x === "string" || Array.isArray(x)) return x.length;
    if (x instanceof Set || x instanceof Map) return x.size;
    if (typeof x === "object") return Object.keys(x).length;
    return 0;
  },
  str(x) {
    if (x === null || x === undefined) return "nalla";
    if (typeof x === "object") { try { return JSON.stringify(x); } catch (_) { return String(x); } }
    return String(x);
  },
  int(x) { const n = Math.trunc(Number(x)); return Number.isFinite(n) ? n : 0; },
  float(x) { const n = Number(x); return Number.isFinite(n) ? n : 0.0; },
  chr(n) { return String.fromCodePoint(Math.trunc(Number(n) || 0)); },
  ord(s) { return String(s || "").codePointAt(0) ?? 0; },
  type(x) {
    if (x === null || x === undefined) return "null";
    if (Array.isArray(x)) return "list";
    return typeof x;
  },
  keys(x) { return (x && typeof x === "object") ? Object.keys(x) : []; },
  values(x) { return (x && typeof x === "object") ? Object.values(x) : []; },
  range(a, b, step = 1) {
    const out = [];
    const start = b === undefined ? 0 : a;
    const end = b === undefined ? a : b;
    for (let i = start; step > 0 ? i < end : i > end; i += step) out.push(i);
    return out;
  },
  mul(a, b) {
    if (typeof a === "string" && typeof b === "number") return a.repeat(Math.max(0, Math.trunc(b)));
    if (typeof b === "string" && typeof a === "number") return b.repeat(Math.max(0, Math.trunc(a)));
    return a * b;
  },
  eq(a, b) {
    if (a === b) return true;
    if ((a === null || a === undefined) && (b === null || b === undefined)) return true;
    return false;
  },
  iter(x) {
    if (x === null || x === undefined) return [];
    if (Array.isArray(x) || typeof x === "string" || x instanceof Set) return x;
    if (typeof x === "object") return Object.keys(x);
    return [];
  },
  get(obj, prop) {
    if (obj === null || obj === undefined) return null;
    const v = obj[prop];
    return v === undefined ? null : v;
  },
  idx(obj, i) {
    if (obj === null || obj === undefined) return null;
    const v = obj[i];
    return v === undefined ? null : v;
  },
  idxSet(obj, i, val) {
    if (obj && i !== undefined) obj[i] = val;
    return val;
  },
  cls(C) {
    return new Proxy(C, {
      apply(target, thisArg, args) { return new target(...args); }
    });
  },
  print(...args) { console.log(...args); },
  call(obj, name, args) {
    if (obj === null || obj === undefined) return null;
    if (typeof obj === "string") {
      switch (name) {
        case "to_lower": case "lower": case "dheere_bol": return obj.toLowerCase();
        case "to_upper": case "upper": case "chilla": return obj.toUpperCase();
        case "find": case "dhund": return obj.indexOf(String(args[0] ?? ""));
        case "startsWith": case "shuru_hota": return obj.startsWith(String(args[0] ?? ""));
        case "endsWith": case "khatam_hota": return obj.endsWith(String(args[0] ?? ""));
        case "slice": return obj.slice(args[0], args[1]);
        case "split": return obj.split(args[0] ?? "");
        case "trim": return obj.trim();
        case "replace": return obj.split(String(args[0] ?? "")).join(String(args[1] ?? ""));
        case "length": case "lambai": return obj.length;
        case "includes": return obj.includes(String(args[0] ?? ""));
        case "number_hai": return /^\d+$/.test(obj);
        case "aage_bhar": return obj.padStart(Number(args[0] || 0), String(args[1] ?? " "));
        case "rang_hata": return obj.replace(/\x1b\[[0-9;]*m/g, "");
      }
    }
    if (Array.isArray(obj)) {
      switch (name) {
        case "push": case "daal": return obj.push(...args);
        case "pop": return obj.pop();
        case "join": case "jod": return obj.join(args[0] ?? ",");
        case "filter": case "chhan": case "chhaan": return obj.filter(args[0]);
        case "map": case "badal": case "sab_pe": return obj.map(args[0]);
        case "reduce": case "kul": case "nichod": return args.length > 1 ? obj.reduce(args[0], args[1]) : obj.reduce(args[0]);
        case "find": case "dhund": return typeof args[0] === "function" ? (obj.find(args[0]) ?? null) : obj.indexOf(args[0]);
        case "some": case "koi": return obj.some(args[0]);
        case "every": case "sab": return obj.every(args[0]);
        case "sort": case "saja": case "line_mein": return [...obj].sort(args[0]);
        case "slice": return obj.slice(args[0], args[1]);
        case "includes": case "has": return obj.includes(args[0]);
        case "length": case "lambai": return obj.length;
        case "pehla": return obj.length ? obj[0] : null;
        case "aakhri": return obj.length ? obj[obj.length - 1] : null;
      }
    }
    if (typeof obj[name] === "function") {
      return obj[name](...args);
    }
    if (typeof obj === "object") {
      if (name === "has") return Object.prototype.hasOwnProperty.call(obj, args[0]);
      if (name === "get") return Object.prototype.hasOwnProperty.call(obj, args[0]) ? obj[args[0]] : (args[1] ?? null);
      if (name === "set") { obj[args[0]] = args[1]; return obj; }
      if (name === "keys") return Object.keys(obj);
      if (name === "values") return Object.values(obj);
      if (name === "length" || name === "lambai") return Object.keys(obj).length;
    }
    return null;
  }
};
