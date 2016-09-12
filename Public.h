#ifndef PUBLIC_H
#define PUBLIC_H



#define INF  999999999
#define PI  3.1415926

void abortError( const int line, const char *file, const char *msg=0);


class Element{
public:
	Element(double val,int inx){value = val; index =inx;}
	bool operator < (const Element &m) const{
		return value < m.value;
    }
	double value;
	int index;
};
#endif
