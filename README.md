# Facebooc

:::info
Because this project is forked from jserv and keeps contributing, 
we checkout to `scc` branch that helps us switching to latest development. 
:::

Proof-of-concept Facebook clone in C.
We minimal the essential dependencies

# Collaboration dependencies
```
sudo apt-get install sassc    \ 
                     cppcheck \
                     aspell   \
                     build-essential \
                     libssl-dev
````

# Prerequisites
Install SQLite and openssl development package:
```bash
sudo apt-get install libsqlite3-dev libssl-dev
```

# Build and Run
```bash
cd facebooc
make run
```

Licensing
---------
`Facebooc` is freely re-distributable under the Apache & BSD License.
Use of this source code is governed by Apache & BSD style license that can be found
in the `LICENSE` file.
