

I was not able to find a single equation for the scale factor in web searches.
So, instead I used equations found on Wikipedia for the matter dominated era, and
the dark energy dominated era:

    Era                      Start       End         Equation
    ---                      -----       ---         --------
    matter dominated         47,000      9.8 Byrs    a = k * t ^ (2/3)
    dark energy dominated    9.8 Byrs    forever     a = k * exp(H * t)

And tuned these equations using the following criteria:
- at t=13.8 (now) a=1
- at t=.00038 (CMB) a=.0009   (2.7 deg k / 3000 deg k)
- hubble paramter values
      age     hubble parameter
      9.8         100
      10.9        90
      12.3        80
      14.0        70
      16.3        60
      19.6        50
- transition from matter dominated to dark energy dominated occurs at 9.8 byrs

First the scale factor for the ranges 13.8 up to 19.6 and 13.8 down to 9.8 byrs 
is calculated, in 1 million year increments, and the results saved in a table.
Starting at t=13.8,a=1; the following is used. H(t) is from linear interpolation
of the values in the table shown above.
    da = H(t) * a * dt

From the preceeding we now know the scale factor at time=9.8. And we also know the scale
factor at time=.00038 is a=.0009 from Wien's displacement law, and that the CMB was
released at time=.00038 and at temperature=3000k. The following equation is solved
for k1 and k2 intersecting these 2 points:
     a = k1 + k2 * time^(2/3)

Scale factors for times > 19.6 are calculated based on a constant Hubble Parameter
equal to 50 (H=50).
    da/dt
    ----- = H
     a

     a               T
     |        da     |
    integral ---- = integral H dt
     |        a      |
    a(19.6)         19.6

    a = exp(H*(T-19.6) + ln(a(19.6)))

We now have scale factor determined for 3 time ranges:
    1:  0.00038 to 9.8      k1 + k2 * t^(2/3)
    2:  9.8 to 19.6         table lookup
    3:  19.6 to infinity    exp(H*(T-19.6) + ln(a(19.6)))

When plotting this scale-factor it is evident that there is a discontinuity
in the slope at t=9.8. To resolve this, range 1 and range 2 are connected using
a line segment and an arc of a circle, starting at T=6.3 and ending at T=12.6.
These time values were emperically determined to yield smooth graphs of the scale 
factor and hubble constant.

So, finally the scale factor calculated by this software is determined for these
4 time ranges (time is in billion years from the big bang):
    1:  0.00038 to 6.3      k1 + k2 * t^(2/3)
    2:  6.3 to 12.6         smooth connection of range 1 and 3
    3:  12.6 to 19.6        table lookup
    4:  19.6 to infinity    exp(H*(T-19.6) + ln(a(19.6)))

