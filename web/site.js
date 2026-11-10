// PulseRun site: pixel headlines from the game's own font, the live-build status strip,
// the build sheet, plates with a pixel loupe, a first-run invitation and the help drawer.
(function () {
  'use strict';
  var reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  var store = {
    get: function (k) { try { return window.localStorage.getItem(k); } catch (e) { return null; } },
    set: function (k, v) { try { window.localStorage.setItem(k, v); } catch (e) { /* private mode */ } }
  };
  function $(id) { return document.getElementById(id); }
  function getJSON(url) {
    return fetch(url, { cache: 'no-cache' }).then(function (r) { if (!r.ok) throw new Error(r.status); return r.json(); });
  }

  // ---------- Pixel headlines: the same 5x7 glyph rows the C++ renderer uses ----------
  var SVGNS = 'http://www.w3.org/2000/svg';
  function glyphSvg(font, text, withShadow) {
    var cols = Math.max(1, text.length * 6 - 1);
    var svg = document.createElementNS(SVGNS, 'svg');
    var pad = withShadow ? 1 : 0;
    svg.setAttribute('viewBox', '0 0 ' + (cols + pad) + ' ' + (7 + pad));
    svg.setAttribute('aria-hidden', 'true');
    var layers = withShadow ? [1, 0] : [0];
    layers.forEach(function (off) {
      for (var i = 0; i < text.length; i++) {
        var rows = font.glyphs[text[i]] || font.glyphs['?'] || [0, 0, 0, 0, 0, 0, 0];
        for (var y = 0; y < 7; y++) {
          var bits = rows[y], x = 0;
          while (x < 5) { // merge horizontal runs into one rect (fewer nodes, same pixels)
            if (bits & (16 >> x)) {
              var start = x;
              while (x < 5 && (bits & (16 >> x))) x++;
              var r = document.createElementNS(SVGNS, 'rect');
              r.setAttribute('x', i * 6 + start + off);
              r.setAttribute('y', y + off);
              r.setAttribute('width', x - start);
              r.setAttribute('height', 1);
              r.style.setProperty('--row', y);
              if (off) r.setAttribute('class', 'shadow');
              svg.appendChild(r);
            } else x++;
          }
        }
      }
    });
    return svg;
  }

  function renderPixelType(font) {
    document.querySelectorAll('[data-px]').forEach(function (el) {
      var lines = el.getAttribute('data-px').split('|');
      var isHead = el.classList.contains('px-head');
      if (isHead) {
        if (!el.getAttribute('aria-label')) el.setAttribute('aria-label', el.textContent.trim());
        el.textContent = '';
        var wrap = document.createElement('span');
        wrap.className = 'px-lines';
        lines.forEach(function (line) {
          var l = document.createElement('span');
          l.className = 'px-line';
          l.appendChild(glyphSvg(font, line, true));
          wrap.appendChild(l);
        });
        el.appendChild(wrap);
        el.classList.add('is-px');
      } else {
        el.appendChild(glyphSvg(font, lines.join(' '), false));
      }
    });
    if (reduceMotion || !('IntersectionObserver' in window)) return;
    var io = new IntersectionObserver(function (entries) {
      entries.forEach(function (e) {
        if (e.isIntersecting) { e.target.classList.add('scan'); io.unobserve(e.target); }
      });
    }, { threshold: 0.3 });
    document.querySelectorAll('.px-head.is-px').forEach(function (h) { io.observe(h); });
  }

  // ---------- Live game status ----------
  var lamp = $('lamp'), status = $('status'), boot = $('boot'), fault = $('fault');
  var ready = false;
  window.addEventListener('pulserun-ready', function () {
    ready = true;
    lamp.dataset.state = 'live';
    status.textContent = 'Live. Click the game, then Space to jump.';
    boot.classList.add('done');
    maybeInvite();
  });
  function showFault() {
    if (ready) return;
    lamp.dataset.state = 'fault';
    status.textContent = 'No signal: the WebAssembly build did not load.';
    boot.classList.add('done');
    fault.hidden = false;
  }
  window.addEventListener('pulserun-fault', showFault);
  setTimeout(showFault, 25000);

  // Keyboard capture only while the game is on screen, so Space still scrolls the page elsewhere.
  var canvas = $('canvas'), helpOpen = false, gameVisible = true;
  function syncKeys() { if (window.Module) window.Module.prKeysActive = gameVisible && !helpOpen; }
  if ('IntersectionObserver' in window) {
    new IntersectionObserver(function (es) {
      gameVisible = es[0].intersectionRatio >= 0.4; syncKeys();
    }, { threshold: [0, 0.4, 1] }).observe(canvas);
  }
  canvas.addEventListener('pointerdown', function () { canvas.focus({ preventScroll: true }); });

  // ---------- First-run invitation (opt-in, shown once) ----------
  var invite = $('invite');
  function maybeInvite() {
    if (store.get('pr.invite.seen')) return;
    invite.hidden = false;
  }
  function closeInvite() { invite.hidden = true; store.set('pr.invite.seen', '1'); }
  invite.querySelector('[data-invite-go]').addEventListener('click', function () {
    closeInvite();
    canvas.scrollIntoView({ behavior: reduceMotion ? 'auto' : 'smooth', block: 'center' });
    canvas.focus({ preventScroll: true });
  });
  invite.querySelector('[data-invite-close]').addEventListener('click', closeInvite);

  // ---------- Help drawer ----------
  var drawer = $('help'), veil = $('help-veil'), lastFocus = null;
  function openHelp() {
    if (helpOpen) return;
    helpOpen = true; syncKeys();
    lastFocus = document.activeElement;
    drawer.hidden = false; veil.hidden = false;
    drawer.focus();
  }
  function closeHelp() {
    if (!helpOpen) return;
    helpOpen = false; syncKeys();
    drawer.hidden = true; veil.hidden = true;
    if (lastFocus && lastFocus.focus) lastFocus.focus();
  }
  document.querySelectorAll('[data-help-open]').forEach(function (b) { b.addEventListener('click', openHelp); });
  document.querySelectorAll('[data-help-close]').forEach(function (b) { b.addEventListener('click', closeHelp); });
  veil.addEventListener('click', closeHelp);
  document.addEventListener('keydown', function (e) {
    if (e.key === '?' && !e.ctrlKey && !e.metaKey) { e.preventDefault(); helpOpen ? closeHelp() : openHelp(); return; }
    if (!helpOpen) return;
    if (e.key === 'Escape') { e.preventDefault(); e.stopPropagation(); closeHelp(); return; }
    if (e.key === 'Tab') { // focus trap
      var f = drawer.querySelectorAll('button, a[href], [tabindex]:not([tabindex="-1"])');
      if (!f.length) return;
      var first = f[0], last = f[f.length - 1];
      if (e.shiftKey && (document.activeElement === first || document.activeElement === drawer)) { e.preventDefault(); last.focus(); }
      else if (!e.shiftKey && document.activeElement === last) { e.preventDefault(); first.focus(); }
    }
  }, true);

  // ---------- Build sheet + plates ----------
  var CAPTIONS = {
    '01_lines': ['Lines', 'DDA and Bresenham in all eight octants, dashed and thick lines', 'src/raster/line.cpp'],
    '02_circles': ['Circles and ellipses', 'Midpoint circle and two-region midpoint ellipse, filled and arcs', 'src/raster/circle.cpp'],
    '03_polygons': ['Polygons', 'Edge-table scanline fill with the even-odd rule, concave shapes', 'src/raster/polygon.cpp'],
    '04_clipping': ['Clipping', 'Cohen-Sutherland lines and Sutherland-Hodgman polygons', 'src/raster/clip.cpp'],
    '05_curves': ['Curves', 'De Casteljau construction for cubic Bezier, Catmull-Rom splines', 'src/raster/curve.cpp'],
    '06_scene_gameplay': ['Gameplay still', 'Every sprite built from the primitives above', 'demos/demo_scenes.cpp'],
    '07_scene_menu': ['Main menu still', 'Logo, ECG pulse line, buttons', 'src/ui/ui.cpp'],
    '08_scene_level_select': ['Level select still', 'Cards with difficulty faces and padlock', 'src/ui/ui.cpp'],
    '09_scene_death': ['Death still', 'Shards clipped to the viewport with Sutherland-Hodgman', 'src/game/sprites.cpp'],
    '10_game_menu': ['Live menu', 'The Checkpoint 1 menu, now animated: the ECG line beats and the cube hops', 'src/game/game_screens.cpp'],
    '11_game_level_select': ['Level select', 'Cards read real best scores; arrow keys lift the focused card', 'src/game/game_screens.cpp'],
    '12_game_settings': ['Settings', 'Hitbox overlay toggle and progress reset', 'src/game/game.cpp'],
    '13_game_play': ['Mid-run', 'The solver playing Level 1 at 43 %, with the Catmull-Rom motion trail', 'src/game/world_render.cpp'],
    '14_game_hitboxes': ['Hitboxes', 'Dashed AABBs are the broad phase; solid hulls are the exact polygons', 'src/game/collision.cpp'],
    '15_game_paused': ['Paused', 'Resume, restart or quit, by keyboard, mouse or touch', 'src/game/game.cpp'],
    '16_game_death_burst': ['Death burst', 'Frame 4 of the 5-frame, 15 fps shock-wave animation', 'src/game/animation.cpp'],
    '17_game_crashed': ['Crashed', 'Progress at death and the best so far', 'src/game/game_screens.cpp'],
    '18_game_complete': ['Level complete', 'Attempts, time and best time', 'src/game/game_screens.cpp']
  };
  function caption(name) {
    var key = name.replace(/\.[a-z]+$/, '');
    if (CAPTIONS[key]) return CAPTIONS[key];
    var words = key.replace(/^\d+_/, '').replace(/_/g, ' ');
    return [words.charAt(0).toUpperCase() + words.slice(1), 'Rendered by the game in headless mode', 'app/'];
  }
  function ago(iso) {
    var s = Math.max(0, (Date.now() - new Date(iso).getTime()) / 1000);
    if (s < 90) return 'just now';
    if (s < 5400) return Math.round(s / 60) + ' min ago';
    if (s < 129600) return Math.round(s / 3600) + ' h ago';
    return Math.round(s / 86400) + ' days ago';
  }
  function link(href, text) { var a = document.createElement('a'); a.href = href; a.textContent = text; return a; }

  function fillBuild(b) {
    var repo = 'https://github.com/' + b.source;
    $('repo-link').href = repo;
    $('build-stamp').textContent = 'main@' + b.short + ' · ' + b.tests;
    $('b-commit').replaceChildren(link(repo + '/commit/' + b.sha, b.short), document.createTextNode(' · ' + b.author));
    $('b-subject').textContent = b.subject;
    var t = $('b-tests'); t.textContent = b.tests; if (/^(\d+)\/\1 /.test(b.tests)) t.className = 'pass';
    $('b-built').textContent = new Date(b.built).toLocaleString() + ' (' + ago(b.built) + ')';
    $('b-run').replaceChildren(link(b.run, 'GitHub Actions run'));
    var grid = $('plate-grid');
    (b.shots || []).forEach(function (name, i) {
      var c = caption(name);
      var fig = document.createElement('figure'); fig.className = 'plate';
      var img = document.createElement('img');
      img.src = 'shots/' + name; img.alt = c[0] + ': ' + c[1]; img.loading = 'lazy'; img.width = 960; img.height = 540;
      var cap = document.createElement('figcaption');
      cap.innerHTML = '<span class="fig mono">Fig. ' + (i + 1) + '</span><b></b> <span></span> <code></code>';
      cap.querySelector('b').textContent = c[0] + '.';
      cap.querySelector('span:not(.fig)').textContent = c[1] + '.';
      cap.querySelector('code').textContent = c[2];
      fig.appendChild(img); fig.appendChild(cap); grid.appendChild(fig);
      attachLoupe(img);
    });
    $('plates-empty').hidden = (b.shots || []).length > 0;
  }

  // ---------- Pixel loupe: magnifies real framebuffer pixels and reads them out ----------
  var loupe = $('loupe'), lc = loupe.querySelector('canvas'), lctx = lc.getContext('2d'), lread = $('loupe-read');
  var N = 21, Z = 8; // 21 x 21 source pixels at 8x
  function attachLoupe(img) {
    var src = null;
    function source() {
      if (src || !img.naturalWidth) return src;
      src = document.createElement('canvas');
      src.width = img.naturalWidth; src.height = img.naturalHeight;
      src.getContext('2d', { willReadFrequently: true }).drawImage(img, 0, 0);
      return src;
    }
    function move(e) {
      var s = source(); if (!s) return;
      var r = img.getBoundingClientRect();
      var px = Math.floor((e.clientX - r.left) / r.width * s.width);
      var py = Math.floor((e.clientY - r.top) / r.height * s.height);
      if (px < 0 || py < 0 || px >= s.width || py >= s.height) { hide(); return; }
      lctx.imageSmoothingEnabled = false;
      lctx.fillStyle = '#0a0b24'; lctx.fillRect(0, 0, lc.width, lc.height);
      var h = (N - 1) / 2;
      lctx.drawImage(s, px - h, py - h, N, N, 0, 0, N * Z, N * Z);
      lctx.strokeStyle = 'rgba(0,0,0,0.35)'; lctx.lineWidth = 1;
      for (var i = 1; i < N; i++) {
        lctx.beginPath(); lctx.moveTo(i * Z + 0.5, 0); lctx.lineTo(i * Z + 0.5, N * Z); lctx.stroke();
        lctx.beginPath(); lctx.moveTo(0, i * Z + 0.5); lctx.lineTo(N * Z, i * Z + 0.5); lctx.stroke();
      }
      lctx.strokeStyle = '#ffffff'; lctx.lineWidth = 2;
      lctx.strokeRect(h * Z, h * Z, Z, Z);
      var d = s.getContext('2d').getImageData(px, py, 1, 1).data;
      var hex = '#' + [d[0], d[1], d[2]].map(function (v) { return ('0' + v.toString(16)).slice(-2); }).join('');
      lread.innerHTML = '<span class="swatch"></span>';
      lread.firstChild.style.background = hex;
      lread.appendChild(document.createTextNode('x ' + px + '  y ' + py + '  ' + hex));
      loupe.hidden = false;
      var lw = loupe.offsetWidth, lh = loupe.offsetHeight;
      var x = e.clientX + 24, y = e.clientY - lh / 2;
      if (x + lw > window.innerWidth - 8) x = e.clientX - lw - 24;
      y = Math.max(8, Math.min(window.innerHeight - lh - 8, y));
      loupe.style.left = x + 'px'; loupe.style.top = y + 'px';
    }
    function hide() { loupe.hidden = true; }
    img.addEventListener('pointermove', move);
    img.addEventListener('pointerdown', move);
    img.addEventListener('pointerleave', hide);
    img.addEventListener('pointercancel', hide);
  }
  window.addEventListener('scroll', function () { loupe.hidden = true; }, { passive: true });

  getJSON('font.json').then(renderPixelType).catch(function () { /* text fallback stays */ });
  // Load the game with the commit SHA as a version, so a new deploy is never hidden behind
  // a cached pulserun.js / pulserun.wasm (GitHub Pages caches files for 10 minutes).
  function loadGame(version) {
    var q = version ? '?v=' + encodeURIComponent(version) : '';
    window.Module.locateFile = function (path) { return 'game/' + path + q; };
    var s = document.createElement('script');
    s.src = 'game/pulserun.js' + q;
    s.async = true;
    s.onerror = function () { window.dispatchEvent(new Event('pulserun-fault')); };
    document.body.appendChild(s);
  }
  fetch('build.json?t=' + Date.now(), { cache: 'no-store' })
    .then(function (r) { if (!r.ok) throw new Error(r.status); return r.json(); })
    .then(function (b) { loadGame(b.short); fillBuild(b); })
    .catch(function () {
      loadGame('');
      $('build-stamp').textContent = 'local preview';
      $('plates-empty').hidden = false;
    });
})();
