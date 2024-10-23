# Scribe - Reliable Serialization for HPC Simulations

## Documentation

Intended for users:
* Using the `Tome` type: [docs/tome.md](docs/tome.md)
* Features of the `Schema` system: [docs/schema.md](docs/schema.md)

For developers or anyone interested:
* Initial design document: [docs/design.md](docs/design.md) (kept roughly up-to-date but not too detailed)
* TODO list for the "1.0" milestone: [docs/mvp_checklist.md](docs/mvp_checklist.md)

## Build Instructions

A typical build on a linux system might look like this:

```bash
# install dependencies
sudo apt install libhdf5-dev

# download/compile Scribe
git https://github.com/krox/Scribe.git
mkdir Scribe/build && cd Scribe/build
cmake ..        # this will fetch further (source) dependencies like nlohmann/json
make

# check everything is in order
./scribe_tests  # run unittests
./scribe --help # check out the main executable

```

## License

This project is licensed under the GNU General Public License v3.0. You are free to use, modify, and distribute this software under the terms of the GPLv3. For more details, see the [COPYING](./COPYING) file or visit https://www.gnu.org/licenses/gpl-3.0.html.


