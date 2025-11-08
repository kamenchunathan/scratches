import { defineConfig } from 'vite'
import tailwindcss from '@tailwindcss/vite'
import { execSync } from 'child_process';
import path from 'path';

const watchAndRunPlugin = (watchPath, command) => ({
  name: 'watch-and-run',
  hotUpdate({ file, type }) {
    if (this.environment?.name !== 'client') {
      return;
    }

    const absoluteFile = path.normalize(file);

    if (type === 'update' && absoluteFile.startsWith(path.normalize(watchPath)) && absoluteFile.endsWith('.purs')) {
      console.log(`(Env: ${this.environment.name}) File changed: ${absoluteFile}. Running command: ${command}`);
      try {
        execSync(command, { stdio: 'inherit' });
      } catch (error) {
        console.error(`Error running command for ${absoluteFile}:`, error);
      }
    }
  },
});

const purescriptSrcPath = path.resolve(__dirname, 'src');
const spagoBuildCommand = 'spago build';

export default defineConfig({
  plugins: [
    tailwindcss(),
    watchAndRunPlugin(purescriptSrcPath, spagoBuildCommand),
  ],
})
