#include "sdf-config.h"
#include "solarpanel.h"
#include "drandom.h"
#include "gccbugs.h"
#include "fpint.h"
#include "time.h"

/**
 * whether initial noise has been calculated
 */
static int noise_init = 0;

/**
 * last time of nouse update
 */
static int noise_last_update_day;

/**
 * noise for complete lifetime
 */
static int noise_lifetime;

/**
 * noise for complete day
 */
static int noise_day;

/**
 * update noise
 */
static void update_noise() {
	// set noise for first time
	if(!noise_init) {
		noise_init            = 1;
		noise_last_update_day = time_day();
		noise_lifetime        = drandom_rand_minmax(-SOLARPANEL_NOISE / 2, SOLARPANEL_NOISE / 2);
		noise_day             = drandom_rand_minmax(-SOLARPANEL_NOISE / 2, SOLARPANEL_NOISE / 2);
	}

	// update noise when a new day begins
	if(noise_last_update_day != time_day()) {
		noise_last_update_day = time_day();
		noise_day             = drandom_rand_minmax(-SOLARPANEL_NOISE / 2, SOLARPANEL_NOISE / 2);
	}
}

/**
 * energy calculated by brock formula in Wh
 *
 * "Calculatingsolar radiation for ecological studies"
 * Ecological Modelling, vol. 14, no. 1-2, pp. 1-19, 1981
 */
static fpint energy_brock(int day, int minute, fpint fp_lat) {
	// right side of brock polynom is equal to left, but:
	// * in simple calculation only range [0,720] is correct, so right side is correct by
	//   transforming to the left side (formula got simpler by approximating only left side)
	if(minute > 720)
		minute = 1440 - minute;

	#if SOLARPANEL_SIMPLECALCULATION
		/**
		 * small but very accurate approximation of day 127
		 * 
		 * interpolating polynomial {278,0}, {388,388}, {499,756.522949}, {720,1111.878159}
		 * calculated by polynom: -9.5455099942671*10^-6*x^3 + 0.0101828*x^2 - 0.0500899*x - 567.954
		 */

		// - 567.954
		long long sum = -568LL;

		// - 0.0500899 x
		long long x = minute;
		sum -= gccbugs_lldiv(x, 20LL);

		// + 0.0101828 x^2
		x*=minute;
		sum += gccbugs_lldiv(x, 98LL);

		// - 9.5455099942671*^-6 x^3
		x*=minute;
		sum -= gccbugs_lldiv(x, 104761LL);

		long fp_sr = fpint_to(sum);
	#else
		fpint fp_day = fpint_to(day);
		fpint fp_minute = fpint_to(minute);

		// radius vector
		fpint fp_rv_cos_term = fpint_div(fpint_mul(fpint_add(FPINT_PI, FPINT_PI), fp_day), 0x16D0000);
		fpint fp_rv_sqrt_term = fpint_add(0x10000, fpint_mul(0x873, fpint_cos(fp_rv_cos_term)));
		fpint fp_rv = fpint_div(0x10000, fp_rv_sqrt_term);

		// declination
		fpint fp_d_numerator = fpint_mul(fpint_add(FPINT_PI, FPINT_PI), fpint_add(fp_day, 0x11C0000));
		fpint fp_d_sin = fpint_sin(fpint_div(fp_d_numerator, 0x16D0000));
		fpint fp_d = fpint_mul(0x68BA, fp_d_sin);

		// minute angle
		fpint fp_ma = fpint_mul(fpint_sub(fp_minute, 0x2D00000), fpint_div(FPINT_PI, 0x2D00000));

		// cosine of zenith angle
		fpint fp_cosza_sinterm = fpint_mul(fpint_sin(fp_d), fpint_sin(fp_lat));
		fpint fp_cosza_costerm = fpint_mul(fpint_cos(fp_d), fpint_mul(fpint_cos(fp_lat), fpint_cos(fp_ma)));
		fpint fp_cosza = fpint_add(fp_cosza_sinterm, fp_cosza_costerm);

		// solar radiation
		long fp_sr = fpint_mul(fpint_div(0x5490000, fpint_mul(fp_rv, fp_rv)), fp_cosza);
	#endif

	return fpint_div(fpint_max(0, fp_sr), fpint_to(SPEEDMULTIPLIER));
}

/**
 * energy calculated for a mote in Wh based on brock with:
 * - scales energy to efficiency
 * - scales energy to size
 * - adds noise
 */
static fpint energy_corrected(fpint fp_energy, int noise) {
	// scale energy to solarpanel size
	fpint fp_squaremeter = 0x27100000; // 1m^2 in cm^2
	fp_energy = fpint_div(fp_energy, fpint_div(fp_squaremeter, fpint_to(SOLARPANEL_SIZE)));

	// reduce harvested energy to solarpanel efficiency
	fpint fp_onehundredpercent = 0x640000;
	fp_energy = fpint_div(fp_energy, fpint_div(fp_onehundredpercent, fpint_to(SOLARPANEL_EFFICIENCY)));

	// random noise in energy
	if(noise != 0) {
		fpint fp_noise = fpint_mul(fpint_to(noise), fpint_div(fp_energy, fp_onehundredpercent));
		fp_energy = fpint_add(fp_energy, fp_noise);
	}

	return fpint_max(0, fp_energy);
}

fpint solarpanel_capacity(long seconds) {
	#if SOLARPANEL_EMULATE
		update_noise();

		// calculate energy of solar panel
		fpint fp_lat = 0xDEDC; // Darmstadt: 49.878667 * PI / 180 = 0.8705
		fpint fp_energy = energy_brock(time_day(), time_minute(), fp_lat);
			  fp_energy = energy_corrected(fp_energy, noise_lifetime + noise_day);

		// convert Watthours to Milliamperhours
		fpint fp_onethousand = 0x3E80000;
		fpint fp_mah = fpint_mul(fpint_div(fp_energy, fpint_to(SOLARPANEL_VOLT)), fp_onethousand);

		// scaled mAh down to timeframe
		fpint fp_hour = 0xE100000; // 1h in seconds
		return fpint_mul(fpint_div(fp_mah, fp_hour), fpint_to(seconds));
	#else
		#error no real solarpanel implemented
	#endif
}
