import { defineConfig } from 'astro/config';

export default defineConfig({
  site: 'https://RafaelReyesCarmona.github.io',
  base: '/SecureGen',
  output: 'static',
  build: {
    assets: 'assets',
  },
});
