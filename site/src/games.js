// The games help page: each game's demo, a numbered list of the board's own
// screens (build.py, demo_player()), becomes a player: one step at a time,
// Back and Next, the arrow keys, a swipe or a tap on the screen, "Step 3 of 8"
// and the step's words read out as they change. It plays by itself while in
// view, unless the reader prefers less motion; it stops when you take over,
// and pauses off screen, on hover and on focus. Without JavaScript the list
// stays as it is. No inline styles or handlers: the site's CSP allows neither.
(() => {
  const still = window.matchMedia('(prefers-reduced-motion: reduce)');
  const EVERY = 3200, LAST = 6000;   // ms on a step, and on the last before it starts again

  for (const demo of document.querySelectorAll('.demo')) {
    const steps = [...demo.querySelectorAll('.demo-step')];
    if (steps.length < 2) continue;
    const name = demo.querySelector('.demo-title')?.textContent.replace(/, step by step$/, '') || 'The game';
    const text = steps.map((li) => li.querySelector('.demo-text')?.textContent.trim() || '');

    const bar = document.createElement('div');
    bar.className = 'demo-controls';
    bar.innerHTML = `
      <button type="button" class="demo-prev" aria-label="Previous step">‹ Back</button>
      <span class="demo-count" aria-hidden="true"></span>
      <button type="button" class="demo-next" aria-label="Next step">Next ›</button>
      <button type="button" class="demo-play"></button>`;
    const caption = document.createElement('p');
    caption.className = 'demo-caption';
    caption.setAttribute('aria-live', 'polite');
    demo.querySelector('.demo-steps').after(caption, bar);
    demo.classList.add('is-player');
    demo.setAttribute('role', 'region');
    demo.setAttribute('aria-label', `${name} demo: use Back and Next, or the arrow keys`);

    const [prev, count, next, play] = ['.demo-prev', '.demo-count', '.demo-next', '.demo-play'].map((s) => bar.querySelector(s));
    let at = 0, playing = false, timer = 0, inView = false, held = false;

    const show = (i, quiet) => {
      at = (i + steps.length) % steps.length;
      steps.forEach((li, n) => li.classList.toggle('on', n === at));
      for (const n of [at, at + 1]) {   // this step and the next load now; the rest when needed
        const img = steps[n % steps.length].querySelector('img');
        if (img) img.loading = 'eager';
      }
      count.textContent = `Step ${at + 1} of ${steps.length}`;
      // while it plays by itself the steps aren't read out one after another
      caption.setAttribute('aria-live', quiet ? 'off' : 'polite');
      caption.innerHTML = '';
      const n = document.createElement('span');
      n.className = 'demo-n';
      n.textContent = `Step ${at + 1} of ${steps.length}. `;
      caption.append(n, text[at]);
      prev.disabled = at === 0;
      next.disabled = at === steps.length - 1;
      demo.classList.remove('flash');
      if (!still.matches) { void demo.offsetWidth; demo.classList.add('flash'); }
    };

    const tick = () => {
      clearTimeout(timer);
      if (!playing || !inView || held || document.hidden) return;
      timer = setTimeout(() => { show(at + 1, true); tick(); }, at === steps.length - 1 ? LAST : EVERY);
    };
    const setPlaying = (on) => {
      playing = on;
      play.textContent = on ? 'Pause' : 'Play';
      play.setAttribute('aria-label', on ? 'Pause the demo' : 'Play the demo');
      tick();
    };
    const go = (i) => { setPlaying(false); show(i); };   // the reader takes over

    prev.addEventListener('click', () => go(at - 1));
    next.addEventListener('click', () => go(at + 1));
    play.addEventListener('click', () => {
      if (!playing && at === steps.length - 1) show(0);
      setPlaying(!playing);
    });
    demo.querySelector('.demo-steps').addEventListener('click', () => go(at === steps.length - 1 ? 0 : at + 1));
    demo.addEventListener('keydown', (e) => {
      const to = { ArrowLeft: at - 1, ArrowRight: at + 1, Home: 0, End: steps.length - 1 }[e.key];
      if (to === undefined || to < 0 || to >= steps.length) return;
      e.preventDefault();
      go(to);
    });
    let x0 = null, y0 = 0;   // a sideways swipe on the screen steps
    demo.addEventListener('touchstart', (e) => {
      x0 = e.touches.length === 1 ? e.touches[0].clientX : null;
      y0 = e.touches[0]?.clientY || 0;
    }, { passive: true });
    demo.addEventListener('touchend', (e) => {
      if (x0 === null) return;
      const dx = e.changedTouches[0].clientX - x0, dy = e.changedTouches[0].clientY - y0;
      x0 = null;
      if (Math.abs(dx) > 40 && Math.abs(dx) > Math.abs(dy)) {
        e.preventDefault();   // not also a tap on the screen
        const to = at + (dx < 0 ? 1 : -1);
        if (to >= 0 && to < steps.length) go(to);
      }
    });
    const hold = (on) => { held = on; tick(); };
    demo.addEventListener('mouseenter', () => hold(true));
    demo.addEventListener('mouseleave', () => hold(false));
    demo.addEventListener('focusin', () => hold(true));
    demo.addEventListener('focusout', (e) => { if (!demo.contains(e.relatedTarget)) hold(false); });
    document.addEventListener('visibilitychange', tick);

    if ('IntersectionObserver' in window) {
      new IntersectionObserver(([e]) => { inView = e.isIntersecting; tick(); }, { threshold: 0.5 }).observe(demo);
    }
    show(0, true);
    setPlaying(!still.matches && 'IntersectionObserver' in window);
    still.addEventListener('change', () => { if (still.matches) setPlaying(false); });
  }
})();
