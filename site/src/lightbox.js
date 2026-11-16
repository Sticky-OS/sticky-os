// Screenshots open in a light box on the page instead of a new page: the
// picture large, its caption under it, the page's other pictures a step away
// (the arrows, the arrow keys, a swipe), and Esc, the X or a tap outside to
// close. Without JavaScript the links still open the picture itself.
// No inline styles or handlers: the site's CSP allows neither.
(() => {
  const pics = [...document.querySelectorAll('.screen img, figure.browser img')]
    .filter((img) => !img.closest('.post-card'));   // a blog card links to its post
  if (!pics.length || typeof HTMLDialogElement !== 'function') return;

  const big = (img) => img.closest('a[href]')?.getAttribute('href') || img.currentSrc || img.src;
  const caption = (img) => img.closest('figure')?.querySelector('figcaption')?.textContent.trim() || '';

  const box = document.createElement('dialog');
  box.className = 'lightbox';
  box.setAttribute('aria-label', 'Picture');
  box.innerHTML = `
    <figure class="lightbox-figure">
      <img class="lightbox-img" alt="">
      <figcaption class="lightbox-caption"></figcaption>
    </figure>
    <button class="lightbox-close" type="button" aria-label="Close">×</button>
    <button class="lightbox-prev" type="button" aria-label="Previous picture">‹</button>
    <button class="lightbox-next" type="button" aria-label="Next picture">›</button>
    <p class="lightbox-count" aria-live="polite"></p>`;
  document.body.append(box);
  const shown = box.querySelector('.lightbox-img'), cap = box.querySelector('.lightbox-caption');
  const count = box.querySelector('.lightbox-count');
  let at = 0;

  const show = (i) => {
    at = (i + pics.length) % pics.length;
    const img = pics[at];
    shown.src = big(img);
    shown.alt = img.alt;
    // Portrait or landscape, as the screen was held: sizes the picture to fit.
    const w = img.naturalWidth || +img.getAttribute('width'), h = img.naturalHeight || +img.getAttribute('height');
    box.classList.toggle('tall', h > w);
    cap.textContent = caption(img);
    count.textContent = pics.length > 1 ? `${at + 1} / ${pics.length}` : '';
    box.classList.toggle('single', pics.length < 2);
  };
  const open = (i) => {
    show(i);
    if (!box.open) box.showModal();
    document.documentElement.classList.add('lightbox-on');
  };
  const close = () => box.open && box.close();
  box.addEventListener('close', () => document.documentElement.classList.remove('lightbox-on'));

  pics.forEach((img, i) => {
    const link = img.closest('a[href]');
    const target = link || img;
    if (!link) {   // a picture with no link of its own: make it a control
      img.tabIndex = 0;
      img.setAttribute('role', 'button');
      img.setAttribute('aria-label', `Open the picture: ${img.alt}`);
    }
    target.classList.add('zoomable');
    target.addEventListener('click', (e) => {
      if (e.metaKey || e.ctrlKey || e.shiftKey || e.button > 0) return;   // a new tab, as asked
      e.preventDefault();
      open(i);
    });
    if (!link) target.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); open(i); }
    });
  });

  box.querySelector('.lightbox-close').addEventListener('click', close);
  box.querySelector('.lightbox-prev').addEventListener('click', () => show(at - 1));
  box.querySelector('.lightbox-next').addEventListener('click', () => show(at + 1));
  box.addEventListener('click', (e) => {   // outside the picture and the buttons: close
    if (e.target === box || e.target.classList.contains('lightbox-figure')) close();
  });
  box.addEventListener('keydown', (e) => {
    if (e.key === 'ArrowLeft') show(at - 1);
    else if (e.key === 'ArrowRight') show(at + 1);
  });
  let x0 = null;   // a swipe sideways steps through the pictures
  box.addEventListener('touchstart', (e) => { x0 = e.touches.length === 1 ? e.touches[0].clientX : null; }, { passive: true });
  box.addEventListener('touchend', (e) => {
    if (x0 === null) return;
    const dx = e.changedTouches[0].clientX - x0;
    if (Math.abs(dx) > 50) show(at + (dx < 0 ? 1 : -1));
    x0 = null;
  });
})();
