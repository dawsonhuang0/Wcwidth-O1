# Wcwidth-O1

[![npm](https://img.shields.io/npm/v/wcwidth-o1.svg)](https://www.npmjs.com/package/wcwidth-o1)

A TypeScript/JavaScript implementation of glibc’s `wcwidth(3)` and `wcswidth(3)`, optimized to *O(1)*.  
Conforms to [POSIX.1-2008 (IEEE Std 1003.1)](https://pubs.opengroup.org/onlinepubs/9699919799/) for 
terminal column width calculation.

### Superior Performance
- ⚡️ Instant *O*(1) lookup time
- 🌏 Full Unicode 17.0 coverage

### References:
- [OpenGroup wcwidth()](https://pubs.opengroup.org/onlinepubs/9699919799/functions/wcwidth.html)  
- [OpenGroup wcswidth()](https://pubs.opengroup.org/onlinepubs/9699919799/functions/wcswidth.html)


## Getting Started

Install Wcwidth-O1 via npm:

```bash
npm i wcwidth-o1
```


## Usage

<h3>JavaScript / TypeScript:</h3>

```ts
import wcwidth from 'wcwidth-o1';

const example1 = wcwidth('a'); // 1
const example2 = wcwidth('好'); // 2
const example3 = wcwidth('😊'); // 2
```

or

```ts
import { wcwidth, wcswidth } from 'wcwidth-o1';

const example = wcwidth('a'); // 1

const example1 = wcswidth('hi'); // 2
const example2 = wcswidth('안녕하세요'); // 10
const example3 = wcswidth('😊こんにちは'); // 12
```

### Function Parameters:

**wcwidth()**:
- `char`: A single-character string to measure.

**wcswidth()**:
- `str`: Input string to evaluate.
- `n`: Max characters to process (defaults to full length).


## Updating Lookup Table

When a new Unicode version is released, the lookup table must be regenerated to follow the latest character width definitions.

### 1. Prerequisites

- glibc-based Linux distro (e.g. Debian).

### 2. Generate new lookup table

```bash
./genTable.sh
```

If your environment is not glibc-based, you'll see:

```bash
[ WARNING ] Please compile on a glibc-based Linux distro (e.g. Debian).
```

Once the generation is complete, you should see:

```bash
[ SUCCESS ] table.ts generated successfully.
```

### 3. Replace files

Copy the generated `table.ts` into `src/`:

```bash
cp table.ts src/
```

The lookup table update is then complete.


## Behind Wcwidth

In fixed-width terminals, most Latin characters take up one column, while East 
Asian (CJK) ideographs usually take up two. The challenge is deciding how many 
“cells” each Unicode character should occupy so that text aligns correctly.

The Unicode standard defines width classes:
- **Wide (W)** and **Fullwidth (F)** - always 2 columns  
- **Halfwidth (H)** and **Narrow (Na)** - always 1 column  
- **Ambiguous (A)** - 1 column normally, but 2 in CJK compatibility mode  
- **Neutral (N)** - treated as 1 column here for simplicity  

Other rules include:
- `U+0000` (null) - width 0  
- Control characters - `-1`  
- Combining marks - width 0  
- Soft hyphen (`U+00AD`) - width 1  
- Zero width space (`U+200B`) - width 0  

This logic originates from Markus Kuhn’s reference implementation and is widely 
used in terminal emulators to ensure consistent alignment.

See [Unicode TR#11](http://www.unicode.org/unicode/reports/tr11/) for more details.


## Feedback

Found something odd?  
Feel free to [open an issue](https://github.com/dawsonhuang0/Wcwidth-O1/issues).


## Acknowledgments

- Original `wcwidth` and `wcswidth` specifications defined by [POSIX.1-2008 (IEEE Std 1003.1)](https://pubs.opengroup.org/onlinepubs/9699919799/functions/wcwidth.html).  
- Behavior and results are derived from the GNU [glibc](https://sourceware.org/glibc/) implementation.  
- Historical reference: [Markus Kuhn’s wcwidth.c](http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c), which inspired most Unicode width logic used today.

## License

Distributed under the MIT License.  
See [`LICENSE`](LICENSE) for more information.
