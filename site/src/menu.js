// The header menu on a narrow screen: a Menu button opens it as a panel under
// the header. Without JavaScript the menu stays one row that scrolls sideways.
const header = document.querySelector('.top');
const nav = header?.querySelector('nav');
if (header && nav) {
  nav.id ||= 'site-menu';
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'menu-toggle';
  button.setAttribute('aria-controls', nav.id);
  button.setAttribute('aria-expanded', 'false');
  button.innerHTML = '<svg width="20" height="20" viewBox="0 0 20 20" aria-hidden="true"><path d="M3 5.5h14M3 10h14M3 14.5h14" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/></svg><span>Menu</span>';
  nav.before(button);
  header.classList.add('has-menu');
  const narrow = window.matchMedia('(max-width: 48rem)');
  const set = (open) => {
    header.classList.toggle('menu-open', open);
    button.setAttribute('aria-expanded', String(open));
    button.querySelector('span').textContent = open ? 'Close' : 'Menu';
  };
  button.addEventListener('click', () => set(!header.classList.contains('menu-open')));
  nav.addEventListener('click', (e) => { if (e.target.closest('a')) set(false); });
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && header.classList.contains('menu-open')) { set(false); button.focus(); }
  });
  document.addEventListener('click', (e) => { if (!header.contains(e.target)) set(false); });
  narrow.addEventListener('change', () => set(false));
}
