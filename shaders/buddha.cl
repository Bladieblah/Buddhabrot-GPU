__constant sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;

/**
 * RNG stuff
 */

__constant ulong PCG_SHIFT = 6364136223846793005ULL;
__constant float PCG_MAX_1 = 4294967296.0;

#define PDF_SIDES 100
#define PDF_SIZE 201

#define UNIFORM_LOW 0.02425
#define UNIFORM_HIGH 0.97575

__constant float a[] = {
    -3.969683028665376e+01,
     2.209460984245205e+02,
    -2.759285104469687e+02,
     1.383577518672690e+02,
    -3.066479806614716e+01,
     2.506628277459239e+00
};

__constant float b[] = {
    -5.447609879822406e+01,
     1.615858368580409e+02,
    -1.556989798598866e+02,
     6.680131188771972e+01,
    -1.328068155288572e+01
};

__constant float c[] = {
    -7.784894002430293e-03,
    -3.223964580411365e-01,
    -2.400758277161838e+00,
    -2.549732539343734e+00,
     4.374664141464968e+00,
     2.938163982698783e+00
};

__constant float d[] = {
    7.784695709041462e-03,
    3.224671290700398e-01,
    2.445134137142996e+00,
    3.754408661907416e+00
};

inline float inverseNormalCdf(float uniform) {
	float q, r;

	if (uniform <= 0) {
		return -HUGE_VAL;
	}
	else if (uniform < UNIFORM_LOW) {
		q = sqrt(-2 * log(uniform));
		return (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
			((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
	}
	else if (uniform <= UNIFORM_HIGH) {
        q = uniform - 0.5;
        r = q * q;
        
        return (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q /
            (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1);
	}
	else if (uniform < 1) {
		q  = sqrt(-2 * log(1 - uniform));

		return -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
			((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
	}
	else {
		return HUGE_VAL;
	}
}

inline ulong pcg32Random(global ulong *randomState, global ulong *randomIncrement, int x) {
    ulong oldstate = randomState[x];
    randomState[x] = oldstate * PCG_SHIFT + randomIncrement[x];
    uint xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint rot = oldstate >> 59u;
    uint pcg = (xorshifted >> rot) | (xorshifted << ((-rot) & 31));

    return pcg;
}

__kernel void seedNoise(
    global ulong *randomState,
    global ulong *randomIncrement,
    global ulong *initState,
    global ulong *initSeq
) {
    const int x = get_global_id(0);

    randomState[x] = 0U;
    randomIncrement[x] = (initSeq[x] << 1u) | 1u;
    pcg32Random(randomState, randomIncrement, x);
    randomState[x] += initState[x];
    pcg32Random(randomState, randomIncrement, x);
}

inline float uniformRand(
    global ulong *randomState,
    global ulong *randomIncrement,
    int x
) {
    return (float)pcg32Random(randomState, randomIncrement, x) / PCG_MAX_1;
}

inline float gaussianRand(
    global ulong *randomState,
    global ulong *randomIncrement,
    int x
) {
    return inverseNormalCdf(uniformRand(randomState, randomIncrement, x));
}

/**
 * Complex math stuff
 */

 inline float2 cmul(float2 a, float2 b) {
    return (float2)( a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
 }

 inline float2 csquare(float2 z) {
    return (float2)( z.x * z.x - z.y * z.y, 2 * z.y * z.x);
 }

 inline float cnorm(float2 z) {
    return z.x * z.x + z.y + z.y;
 }

/**
 * Coordinate transformations
 */

 typedef struct ViewSettings {
    float scaleX, scaleY;
    float centerX, centerY;
    float theta, sinTheta, cosTheta;
    int sizeX, sizeY;
} ViewSettings;

inline float2 rotateCoords(float2 coords, ViewSettings view) {
    return (float2) {
        view.cosTheta * coords.x - view.sinTheta * coords.y,
        view.sinTheta * coords.x + view.cosTheta * coords.y
    };
}

// Transform fractal coordinates to screen coordinates, following openGL conventions
// The viewport spans [-1,1] in both dimensions
inline float2 fractalToScreen(float2 fractalCoord, ViewSettings view) {
    float2 tmp = rotateCoords((float2) {
        (fractalCoord.x - view.centerX),
        (fractalCoord.y - view.centerY)
    }, view);

    tmp.x = tmp.x / view.scaleX;
    tmp.y = tmp.y / view.scaleY;

    return tmp;
}

inline int2 screenToPixel(float2 screenCoord, ViewSettings view) {
    return (int2) {
        (1 + screenCoord.x) / 2 * view.sizeX,
        (1 + screenCoord.y) / 2 * view.sizeY
    };
}

 inline int2 fractalToPixel(float2 fractalCoord, ViewSettings view) {
    return screenToPixel(fractalToScreen(fractalCoord, view), view);
 }

/**
 * Fractal stuff
 */

typedef struct Particle {
    float2 pos;
    float2 offset, prevOffset;
    unsigned int iterCount;
    float score, prevScore;
} Particle;

__kernel void resetCount(global unsigned int *count, int size) {
    const int x = get_global_id(0);

    for (int i = 0; i < size; i++) {
        count[size * x + i] = 0;
    }
}

inline int matchThreshold(
    Particle particle,
    global unsigned int *threshold,
    int thresholdCount
) {
    for (int i = 0; i < thresholdCount; i++) {
        if (particle.iterCount <= threshold[i]) {
            return i;
        }
    }

    return -1;
}

inline void addPath(
    Particle *particle,
    global float2 *path,
    global unsigned int *count,
    global unsigned int *threshold,
    int thresholdCount,
    unsigned int pathStart,
    int thresholdIndex,
    ViewSettings view
) {
    unsigned int pixelCount = view.sizeX * view.sizeY;
    float deltaScore = 1. / (float)particle->iterCount;
    
    for (unsigned int i = 0; i < particle->iterCount; i++) {
        int2 pixel = fractalToPixel(path[pathStart + i], view);

        if (! (pixel.x < 0 || pixel.x >= view.sizeX || pixel.y < 0 || pixel.y >= view.sizeY)) {
            atomic_inc(&count[thresholdIndex * pixelCount + view.sizeX * pixel.y + pixel.x]);
            particle->score += deltaScore;
        }

        path[pathStart + i].y = -path[pathStart + i].y;
        pixel = fractalToPixel(path[pathStart + i], view);

        if (! (pixel.x < 0 || pixel.x >= view.sizeX || pixel.y < 0 || pixel.y >= view.sizeY)) {
            atomic_inc(&count[thresholdIndex * pixelCount + view.sizeX * pixel.y + pixel.x]);
        }
    }

    particle->score = pown(particle->score, 2);
}

inline void resetParticle(
    Particle *particle,
    global float2 *path,
    unsigned int pathStart,
    global ulong *randomState,
    global ulong *randomIncrement,
    int x
) {
    float2 newOffset = (float2)(
        (9. * uniformRand(randomState, randomIncrement, x) - 5.2),
        (6. * uniformRand(randomState, randomIncrement, x) - 3.)
    );

    particle->iterCount = 1;
    particle->pos = newOffset;
    particle->offset = newOffset;
    particle->prevOffset = newOffset;
    particle->score = 0;
    particle->prevScore = 0;

    path[pathStart] = newOffset;
}

inline void mutateParticle(
    Particle *particle,
    global float2 *path,
    unsigned int pathStart,
    global ulong *randomState,
    global ulong *randomIncrement,
    int x,
    ViewSettings view
) {
    if (particle->prevScore == 0 && particle->score == 0) {
        resetParticle(particle, path, pathStart, randomState, randomIncrement, x);
    } else if (particle->score > particle->prevScore || particle->score / particle->prevScore > uniformRand(randomState, randomIncrement, x)) {
        particle->prevScore = particle->score;
        particle->prevOffset = particle->offset;
    }

    float2 newOffset = (float2)(
        particle->prevOffset.x + 0.05 * view.scaleY * gaussianRand(randomState, randomIncrement, x),
        particle->prevOffset.y + 0.05 * view.scaleY * gaussianRand(randomState, randomIncrement, x)
    );

    particle->iterCount = 1;
    particle->pos = newOffset;
    particle->offset = newOffset;
    particle->score = 0;

    path[pathStart] = newOffset;
}

__kernel void initParticles(
    global Particle *particles,
    global unsigned int *threshold,
    global float2 *path,
    global ulong *randomState,
    global ulong *randomIncrement,
    int thresholdCount
) {
    const int x = get_global_id(0);

    Particle tmp = particles[x];
    resetParticle(&tmp, path, x * threshold[thresholdCount - 1], randomState, randomIncrement, x);
    particles[x] = tmp;
}

__kernel void mandelStep(
    global Particle *particles,
    global unsigned int *count,
    global unsigned int *threshold,
    global float2 *path,
    global ulong *randomState,
    global ulong *randomIncrement,
    int thresholdCount,
    ViewSettings view
) {
    const int x = get_global_id(0);
    const unsigned int maxLength = threshold[thresholdCount - 1];
    const unsigned int pathIndex = x * maxLength;

    Particle tmp = particles[x];

    for (int i = 0; i < 800; i++) {
        tmp.pos = csquare(tmp.pos) + tmp.offset;
        path[pathIndex + tmp.iterCount] = tmp.pos;
        tmp.iterCount++;

        tmp.pos = csquare(tmp.pos) + tmp.offset;
        path[pathIndex + tmp.iterCount] = tmp.pos;
        tmp.iterCount++;

        tmp.pos = csquare(tmp.pos) + tmp.offset;
        path[pathIndex + tmp.iterCount] = tmp.pos;
        tmp.iterCount++;

        tmp.pos = csquare(tmp.pos) + tmp.offset;
        path[pathIndex + tmp.iterCount] = tmp.pos;
        tmp.iterCount++;

        tmp.pos = csquare(tmp.pos) + tmp.offset;
        path[pathIndex + tmp.iterCount] = tmp.pos;
        tmp.iterCount++;

        if (fabs(tmp.pos.x) > 4 || fabs(tmp.pos.y) > 4 || cnorm(tmp.pos) > 16) {
            int thresholdIndex = matchThreshold(tmp, threshold, thresholdCount);
            addPath(&tmp, path, count, threshold, thresholdCount, pathIndex, thresholdIndex, view);
            mutateParticle(&tmp, path, pathIndex, randomState, randomIncrement, x, view);
        }

        if (tmp.iterCount >= maxLength) {
            mutateParticle(&tmp, path, pathIndex, randomState, randomIncrement, x, view);
        }
    }

    particles[x] = tmp;
}

/**
 * Global operations, to be optimised later
 */

__kernel void findMax1(global unsigned int *count, global unsigned int *maxima, unsigned int size) {
    const int x = get_global_id(0);
    maxima[x] = 0;

    for (int i = x * size; i < (x + 1) * size; i++) {
        if (count[i] > maxima[x]) {
            maxima[x] = count[i];
        }
    }
}

__kernel void findMax2(global unsigned int *maxima, global unsigned int *maximum, unsigned int size) {
    const int x = get_global_id(0);
    maximum[x] = 0;

    for (int i = x * size; i < (x + 1) * size; i++) {
        if (maxima[i] > maximum[x]) {
            maximum[x] = maxima[i];
        }
    }
}

/**
 * Rendering
 */

__constant float COLOR_SCHEME[4][3] = {
    {0.1, 0.0, 0.2,},
    {0.0, 0.5, 0.6,},
    {0.8, 0.5, 0.0,},
    {0.1, 0.0, 0.2,},
};

__constant float IMAGE_MAX = 4294967295.0;

 __kernel void renderImage(
    global unsigned int *count,
    global unsigned int *maximum,
    global unsigned int *image,
    int thresholdCount
) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    const int W = get_global_size(0);
    const int H = get_global_size(1);

    const unsigned int pixelOffset = W * y + x;
    unsigned int pixelCount = W * H;
    const unsigned int imageOffset = 3 * pixelOffset;

    for (int j = 0; j < 3; j++) {
        image[imageOffset + j] = 0;

        for (int i = 0; i < thresholdCount; i++) {
            float countFraction = (float)count[i * pixelCount + pixelOffset] / (float)maximum[i];
            image[imageOffset + j] += (int)(COLOR_SCHEME[i][j] * sqrt(countFraction) * IMAGE_MAX);
        }

        if (image[imageOffset + j] >= 4294967295) {
           image[imageOffset + j] = 4294967295;
        }
    }
}
