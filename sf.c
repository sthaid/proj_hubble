

I was not able to find a single equation for the scale factor in web searches.
So, instead I used equations found on Wikipedia for the matter dominated era, and
the dark energy dominated era:

    Era                      Start       End         Equation
    ---                      -----       ---         --------
    matter dominated         47,000      9.8 Byrs    a = k * t ^ (2/3)
    dark energy dominated    9.8 Byrs    forever     a = k * exp(H * t)

And tuned these equations using the following criteria:
- at t=13.8 (now) a=1
- at t=.00038 (cmb) a=.0009   (2.7 degk / 3000 degk)
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
is calculated, in 1 million year increments, and the result saved in a table.
Starting at t=13.8,a=1; this is used:
    da = H(t) * a * dt

From the above we now know the scale factor at time=9.8. And we also know the scale
factor at time=.00038 is a=.0009 from Wien's displacement law, and that the CMB was
released at time=.00038 and at temperature=3000k. The following equation is solved
for k1 and k2 using these 2 points:
     a = k1 + k2 * time^(2/3)

Scale factors for times > 19.6 are calculated based on a constant Hubble Parameter
equal to 50.

So, we now have scale factor determined for 3 time ranges:
    .00038 to 9.8       k1 + k2 * t^(2/3)
    9.8 to 19.6         table lookup
    19.6 to infinity    




Combinining these
The 2 equations do not 





