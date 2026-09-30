/**
 * Let Node load a TypeScript project that imports its own emitted ".js".
 *
 * Node strips types from a .ts file happily, but the sources say
 * `from "./PacketParser.js"` -- the name the build would emit -- and nothing
 * remaps that back. This hook does, so the checkout can be read as-is without
 * a build step. Used by crosscheck.mjs:
 *
 *     node --import ./Tools/t2/tsresolve.mjs Tools/t2/crosscheck.mjs ...
 */
import { registerHooks } from "node:module";
import { existsSync } from "node:fs";
import { fileURLToPath } from "node:url";

registerHooks({
  resolve(specifier, context, next) {
    if (specifier.startsWith(".") && specifier.endsWith(".js")) {
      try {
        const asTs = new URL(specifier.slice(0, -3) + ".ts", context.parentURL);
        if (existsSync(fileURLToPath(asTs))) {
          return next(specifier.slice(0, -3) + ".ts", context);
        }
      } catch {
        // fall through to the normal resolution
      }
    }
    return next(specifier, context);
  },
});
