#include <unistd.h>
#include <gsm/gsm.h>
#include <glib.h>

gboolean stop;

#define NSAMP 160

int
sound_encode (int infd, int outfd)
{
  gushort indata[NSAMP];
  gushort indata1[2*NSAMP];
  gsm_frame outdata;
  size_t r;
  gsm g = gsm_create ();

  do {
    guint i;

    r = read (infd, indata1, sizeof (indata1));
    if (r < 0)
      {
	gsm_destroy (g);
	return -1;
      }

    if (r < sizeof (indata1))
      memset (indata1 + r, 0, sizeof(indata1) - r);

    for (i = 0; i < NSAMP; i++)
      indata[i] = indata1[2*i];

    gsm_encode (g, indata, outdata);
    if (write (outfd, outdata, sizeof (outdata)) < sizeof (outdata))
      {
	gsm_destroy (g);
	return -1;
      }
  } while (r = sizeof (indata1) && !stop);

  gsm_destroy (g);

  return 0;
}

int
sound_decode (int infd, int outfd)
{
  gushort outdata[NSAMP];
  gushort outdata1[NSAMP * 2];
  gsm_frame indata;
  size_t r;
  gsm g = gsm_create ();

  do {
    guint i;

    r = read (infd, indata, sizeof (indata));
    if (r != sizeof (indata))
      {
	gsm_destroy (g);
	return r ? -1 : 0;
      }

    gsm_decode (g, indata, outdata);

    for (i = 0; i < NSAMP; i++)
      {
	outdata1[2*i] = outdata[i];
	outdata1[2*i+1] = outdata[i];
      }
    if (write (outfd, outdata1, sizeof (outdata1)) < sizeof (outdata1))
      {
	gsm_destroy (g);
	return -1;
      }
  } while (!stop);

  gsm_destroy (g);

  return 0;
}
