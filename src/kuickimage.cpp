#include "kuickimage.h"

KuickImage::KuickImage( const KuickFile * file, ImlibImage *im, ImlibData *id)
    : QObject( 0L, 0L )
{
    myFile     = file;
    myIm       = im;
    myId       = id;
    myPixmap   = 0L;
    myWidth    = im->rgb_width;
    myHeight   = im->rgb_height;
    myIsDirty  = true;

    myOrigWidth  = myWidth;
    myOrigHeight = myHeight;
    myRotation   = ROT_0;
    myFlipMode   = FlipNone;
}

KuickImage::~KuickImage()
{
    if ( myPixmap )
        Imlib_free_pixmap( myId, myPixmap );
    Imlib_destroy_image( myId, myIm );
}


void KuickImage::resize( int width, int height )
{
    if ( myWidth == width && myHeight == height )
	return;

    myWidth   = width;
    myHeight  = height;
    myIsDirty = true;
}


void KuickImage::restoreOriginalSize()
{
    if (myWidth == myOrigWidth && myHeight == myOrigHeight)
	return;

    myWidth   = myOrigWidth;
    myHeight  = myOrigHeight;
    myIsDirty = true;

    if ( myRotation == ROT_90 || myRotation == ROT_270 )
        qSwap( myWidth, myHeight );
}


Pixmap& KuickImage::pixmap()
{
    if ( myIsDirty )
	renderPixmap();

    return myPixmap;
}


void KuickImage::renderPixmap()
{
    if ( !myIsDirty )
	return;

//     qDebug("### rendering: %s", myFilename.latin1());

    if ( myPixmap )
	Imlib_free_pixmap( myId, myPixmap );

    emit startRendering();

// #ifndef NDEBUG
//     struct timeval tms1, tms2;
//     gettimeofday( &tms1, NULL );
// #endif

    Imlib_render( myId, myIm, myWidth, myHeight );
    myPixmap = Imlib_move_image( myId, myIm );

// #ifndef NDEBUG
//     gettimeofday( &tms2, NULL );
//     qDebug("*** rendering image: %s, took %ld ms", myFilename.latin1(),
//            (tms2.tv_usec - tms1.tv_usec)/1000);
// #endif


    emit stoppedRendering();

    myIsDirty = false;
}


void KuickImage::rotate( Rotation rot )
{
    if ( rot == ROT_180 ) { 		// rotate 180 degrees
	Imlib_flip_image_horizontal( myId, myIm );
	Imlib_flip_image_vertical( myId, myIm );
    }

    else if ( rot == ROT_90 || rot == ROT_270 ) {
	qSwap( myWidth, myHeight );
	Imlib_rotate_image( myId, myIm, -1 );

	if ( rot == ROT_90 ) 		// rotate 90 degrees
	    Imlib_flip_image_horizontal( myId, myIm );
	else if ( rot == ROT_270 ) 		// rotate 270 degrees
	    Imlib_flip_image_vertical( myId, myIm );
    }

    myRotation = (Rotation) ((myRotation + rot) % 4);
    myIsDirty = true;
}


bool KuickImage::rotateAbs( Rotation rot )
{
    if ( myRotation == rot )
	return false;

    int diff = rot - myRotation;
    bool clockWise = (diff > 0);

    switch( abs(diff) ) {
    case ROT_90:
        rotate( clockWise ? ROT_90 : ROT_270 );
	break;
    case ROT_180:
	rotate( ROT_180 );
	break;
    case ROT_270:
        rotate( clockWise ? ROT_270 : ROT_90 );
	break;
    }

    return true;
}

void KuickImage::flip( FlipMode flipMode )
{
    if ( flipMode & FlipHorizontal )
	Imlib_flip_image_horizontal( myId, myIm );
    if ( flipMode & FlipVertical )
	Imlib_flip_image_vertical( myId, myIm );

    myFlipMode = (FlipMode) (myFlipMode ^ flipMode);
    myIsDirty = true;
}

bool KuickImage::flipAbs( int mode )
{
    if ( myFlipMode == mode )
	return false;

    bool changed = false;

    if ( ((myFlipMode & FlipHorizontal) && !(mode & FlipHorizontal)) ||
	 (!(myFlipMode & FlipHorizontal) && (mode & FlipHorizontal)) ) {
	Imlib_flip_image_horizontal( myId, myIm );
	changed = true;
    }

    if ( ((myFlipMode & FlipVertical) && !(mode & FlipVertical)) ||
	 (!(myFlipMode & FlipVertical) && (mode & FlipVertical)) ) {
	Imlib_flip_image_vertical( myId, myIm );
	changed = true;
    }

    if ( changed ) {
        myFlipMode = (FlipMode) mode;
        myIsDirty = true;
        return true;
    }

    return false;
}

#if 0
QImage KuickImage::smoothTransform()
{
	int w=myViewport.width();
	int h=myViewport.height();

	QImage dst(myViewport.width(), myViewport.height(), myIm->depth(), myIm->numColors(), myIm->bitOrder());
	
	QRgb *scanline;
	
	int basis_ox, basis_oy, basis_xx, basis_yy;
	
	double scalex=myWidth/(double) myIm->width();
	double scaley=myHeight/(double) myIm->height();
	
	basis_ox=(int) (myViewport.left()*4096.0/scalex);
	basis_oy=(int) (myViewport.top()*4096.0/scaley);
	basis_xx=(int) (4096.0/scalex);
	basis_yy=(int) (4096.0/scaley);

	//qDebug("Basis: (%d, %d), (%d, 0), (0, %d)", basis_ox, basis_oy, basis_xx, basis_yy);
		
	int x2, y2;
	
	int max_x2=(myIm->width()<<12);
	int max_y2=(myIm->height()<<12);
	
	QRgb background=idata->backgroundColor.rgb();
	
	QRgb **imdata=(QRgb **) myIm->jumpTable();
	
	int y=0;
	
	for (;;)	//fill the top of the target pixmap with the background color
	{
		y2=basis_oy+y*basis_yy;
		
		if ((y2 >= 0  && (y2 >> 12) < myIm->height()) || y >= h)
			break;
		
		scanline=(QRgb*) dst.scanLine(y);
		for (int i=0; i < w; i++)
			*(scanline++)=background; //qRgb(0,255,0);
		y++;
	}
	
	for (; y < h; y++)
	{
		scanline=(QRgb*) dst.scanLine(y);
	
		x2=basis_ox;
		y2=basis_oy+y*basis_yy;
	
		if (y2 >= max_y2)
			break;
		
		int x=0;
		
		while  ((x2 < 0 || (x2>>12) >= myIm->width()) && x < w)	//fill the left of the target pixmap with the background color
		{
			*(scanline++)=background; //qRgb(0,0,255);
			x2+=basis_xx;
			x++;
		}
		
		int top=y2 >> 12;
		int bottom=top+1;
		if (bottom >= myIm->height())
			bottom--;						
				
		for (; x < w; x++)
		{
			int left=x2 >> 12;
			
			
			int right=left+1;
			
			if (right >= myIm->width())
				right=myIm->width()-1;

			unsigned int wx=x2  & 0xfff; //12 bits of precision for reasons which will become clear
			unsigned int wy=y2 & 0xfff; //12 bits of precision 
			
			unsigned int iwx=0xfff-wx;
			unsigned int iwy=0xfff-wy;
			
			QRgb tl, tr, bl, br;
			

			tl=imdata[top][left];
			tr=imdata[top][right];
			bl=imdata[bottom][left];
			br=imdata[bottom][right];

			/*			
			tl=getValidPixel(myIm, left, top, x, y);		//these calls are expensive
			tr=getValidPixel(myIm, right, top, x, y);		//use them to debug segfaults in this function
			bl=getValidPixel(myIm, left, bottom, x, y);
			br=getValidPixel(myIm, right, bottom, x, y);
			*/
			
			unsigned int r=(unsigned int) (qRed(tl)*iwx*iwy+qRed(tr)*wx*iwy+qRed(bl)*iwx*wy+qRed(br)*wx*wy); // NB 12+12+8 == 32
			unsigned int g=(unsigned int) (qGreen(tl)*iwx*iwy+qGreen(tr)*wx*iwy+qGreen(bl)*iwx*wy+qGreen(br)*wx*wy);
			unsigned int b=(unsigned int) (qBlue(tl)*iwx*iwy+qBlue(tr)*wx*iwy+qBlue(bl)*iwx*wy+qBlue(br)*wx*wy);
			
			*(scanline++)=qRgb(r >> 24, g >> 24, b >> 24); //we're actually off by one in 255 here
			
			x2+=basis_xx;
			
			if (x2 > max_x2)
			{
				x++;
				break;
			}
				
		}
		
		while  (x < w)	//fill the right of each scanline with the background colour
		{
			*(scanline++)=background; //qRgb(255,0,0);
			x++;
		}
	}
	
	for (;;)	//fill the bottom of the target pixmap with the background color
	{
		y2=basis_oy+y*basis_yy;
		
		if (y >= h)
			break;
		
		scanline=(QRgb*) dst.scanLine(y);
		for (int i=0; i < w; i++)
			*(scanline++)=background; //qRgb(255,255,0);
		y++;
	}
	
	return dst.copy();
}
#endif

#include "kuickimage.moc"
