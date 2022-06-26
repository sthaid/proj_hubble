# Expanding Universe UNDER CONSTRUCTION

This project contains two programs (cmb & galaxy), which simulate the cosmic background
radiation, and the visibility of galaxies. The time range of this simulation is
from 0.000380 to 200 billion years after the big bang.

These simulations are largely based on the Cosmic Scale Factor, refer to
[Wikipedia Cosmic Scale Factor](https://en.wikipedia.org/wiki/Scale_factor_(cosmology)).

## Hubble Parameter

The Hubble Parameter is related to the Scale Factor as follows:

```
           d a(t) / dt
    H(t) = -----------
              a(t)
```

Where:
* H(t) is the Hubble Parameter (units: /sec)
* a(t) is the Scale Factor (dimensionless)

The value of the Scale Factor now is defined to be 1.

The value of the Hubble Parameter now is approximately 70 km/s/Mpc, and 
is called the Hubble Constant. The units used for the Hubble Parameter are most often
km/sec/Mpc (kilometers per second per megaparsec). The SI units for the Hubble Parameter
are '/sec'.

## Key Events In Our Universe

This simulation covers times starting at 0.00038 byr. Key events prior to 0.00038 byr are not included here.

* 0.00038 byr: The ionized plasma recombined to form atoms of Hydrogen and Helium, which allowed
photons to travel freely. The universe transitioned from opaque to transparent. The black body
temperature of the photons is 3000 degrees K.

* 1 byr: Galaxies are forming.

* 9.8 byr: The expansion of the universe transitioned from being dominated by matter density to
dark energy. When this transition completes, at about time 20 byr, the size of the universe
will increase exponentially with time.

* 13.8 byr: Now. We can still see the CMB photons from time 0.00038 byr. The
universe has expanded in size by a factor of 1100 since time .000038, which expanded the
wavelength of the CMB by a factor of 1100, and reduced the temperature by a factor of 1100.
The temperature of the CMB is now 2.7 degrees K. The Hubble Parameter is now 70 km/s/Mpc.

* 20 byr: The value of the Hubble Parameter is 50 km/s/Mpc; and the Hubble Parameter will 
remain constant at 50 km/s/Mpc into the future. The size (scale factor) of the universe is increasing
exponentially.

## Assumptions

The Earth is assumed to be a perpetual observation station, existing from time
0.00038 byr to the distant future.

The univers is flat, homogeneous and isotropic. The simulation makes no attempt to simulate
fluctuations in the CMB, or in the distribution of galaxies.

Galaxies are not moving. The apparent motion of galaxies is entirely due to the Hubble Flow.

## Cmb Program

This program simulates the position of CMB photons and the patch of space from which these
photon originated. The simulation is from time 0.00038 byr to T_DONE. The starting distance
of the photons from the perpetual earth observation station is chosen so that the photons
will arrive at the earth at time T_DONE.

In this example [CMB Simulation](https://youtu.be/TjxFwbwMWJ8) T_DONE is set to 13.8.
The initial distance (at T=0.00038) of the CMB photons, and the patch of space from which these
photons originated, is determined by the program to be 0.042 blyr from the earth.
As the simulation runs the distance of the photons increases to a maximum of 5.9 blyr at time
4.5 byr, this is because the Hubble flow exceeds the speed of light. As the universe ages
the Hubble Parameter decreases (reducing the Hubble flow), and the photons reduce their distance
to Earth, eventually arriving at Earth at time 13.8 byr.

The distance of the patch of space (from which the photons were emitted at time 0.00038) to the 
Earth is constantly increasing due to the Hubble flow. At time T_DONE this patch of space has moved
to a distance of 46.4 blyr. Doubling this gives a diameter of the observable universe of 92.8 blyr.

Another example, [CMB Simulation](https://youtu.be/jiRkqq_4a3Q),
has T_DONE is set to 200 byr.

Program Controls:
* Click the light blue RUN, PAUSE, RESUME, RESET to control the simulation; or use the 'x' and 'r' keys.
* Prior to running the T_DONE can be set using the left/right arrow keys, or shift left/right arrow keys.
* The display width can be adjusted using the up/down or shift-up/down arrow keys.

Program Display:
* Main pane:
  * Yellow background indicates the temperature of the CMB.
  * Light blue circle at center is the Earth observation station.
  * Red dots are the photons.
  * Red circle is the patches of space from which the CMB photons were emitted.
* Graph panes: Scale Factor, Hubble Parameter, CMB Temperature, and Photon Distance to Earth.

## Galaxy TODO

describe:
* light blue circle in ctr
* CMB temperature, yellow
* Red circle - location of space from which the photons were emitted
* blue pts
* white pts
* 4 graphs

program controls
* -t_max
* etc
