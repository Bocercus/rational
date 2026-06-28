#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <new>
#include <vector>
#include <stdint.h>
#include <fstream>
#include <stdexcept>
#include <chrono>

typedef uint8_t word;
typedef uint16_t dblword;
#define wordones 0xff
#define wordbits 8

inline int iabs(int x) {return (x<0?-x:x);}
inline int ramp(int x) {return (x>0?x:0);}

//bool debugbool;

class dynsiz{
    public:
    std::vector<word> ds; //Number array ( ds[0] least significant figure )
    dynsiz* div_remp; //Pointer to remainder of integer division
    dynsiz(int n){
        ds.assign(n,0);
        div_remp=nullptr;
    }
    ~dynsiz(){
        delete div_remp;
    }
    /* AI-written start */
    dynsiz(const dynsiz& x){
        //printf("Copy constructor\n");
        ds=x.ds;
        if(x.div_remp)
            div_remp = new dynsiz(*x.div_remp);
        else
            div_remp = nullptr;
    }
     // Copy assignment (strong exception safety)
    dynsiz& operator=(const dynsiz& x){
        //printf("Copy assignment\n");
        if (this == &x) return *this;
        ds = x.ds;
        delete div_remp;
        div_remp = x.div_remp ? new dynsiz(*x.div_remp) : nullptr;
        return *this;
    }
    // Move constructor
    dynsiz(dynsiz&& other) noexcept : ds(std::move(other.ds)), div_remp(other.div_remp) {
        //printf("Move constructor\n");
        other.div_remp = nullptr;
    }
    // Move assignment
    dynsiz& operator=(dynsiz&& other) noexcept {
        //printf("Move assignment\n");
        if (this != &other) {
            ds = std::move(other.ds);
            delete div_remp;
            div_remp = other.div_remp;
            other.div_remp = nullptr;
        }
        return *this;
    }
    /* AI-written end */
    bool expandSize(int n){
        for(int i=0;i<n;i++)
            ds.push_back(0);
        return true;
    }
    void printNums(){
        for(int i=0;i<ds.size();i++)
            printf("%d ",ds[i]); //%x
        printf("\n");
    }
    void printHex(){
        printf("0x");
        for(int i=0;i<ds.size();i++){
            if(ds[ds.size()-i-1]<16)
                printf("0");
            printf("%x",ds[ds.size()-i-1]);
        }
        printf("\n");
    }
    void ensureEqLen(dynsiz* bptr){//Ensure pointed synsize have equal size as this object
        if(bptr->ds.size()>ds.size()) expandSize(bptr->ds.size()-ds.size()); //while(b.siz>siz) expandSize(1);
        if(bptr->ds.size()<ds.size()) bptr->expandSize(ds.size()-bptr->ds.size()); //while(b.siz<siz) b.expandSize(1);
        //Appearently returning a bool from this functions breaks something (when void->bool ofc)
    }
    void operator<<=(word bs){//bs: amount of bits to be shifted
        if(bs>wordbits){//We shall not shift more than an extra word 
            printf("Bitshift too large (%d)\n",bs);
            return;
        }
        std::vector<dblword> bsds;
        int i;
        for(i=0;i<ds.size();i++){
            bsds.push_back(ds[i]); //static_cast<dblword>(ds[i]);
            bsds[i]<<=bs; //Actual shift
            ds[i] = 0;
        }
        for(i=0;i<ds.size();i++){
            ds[i] |= bsds[i] & wordones; //Setting shifted least-significant-word
            if(i>0)
                ds[i] |= (bsds[i-1]>>wordbits); //Setting carry to final output
        }
        if(bsds[ds.size()-1] & 0xff00){ //If carry for most-significant bit is unzero add bit
            expandSize(1);
            ds[ds.size()-1] = (bsds[ds.size()-2]>>wordbits); 
        }
    }
    private:
    inline void pm(dynsiz b,bool plus){
        ensureEqLen(&b);
        word sum;
        word c=0;
        for(int i=0;i<ds.size();i++){
            if(plus){
                sum=ds[i]+b.ds[i]+c; c=0;
                if(sum<ds[i]) c=1;
            }else{
                sum=ds[i]-b.ds[i]-c; c=0;
                if(sum>ds[i]) c=1;
            }
            //printf("sum: %d, c: %d\n",sum,c);
            ds[i]=sum;
        }
        if(c==1){
            expandSize(1);
            ds[ds.size()-1]=1;
        }
    }
    public:
    void operator+=(dynsiz& b){ pm(b,true); }
    void operator-=(dynsiz& b){ pm(b,false); }
    void operator*=(dynsiz& b){
        //printNums();printf("* ");b.printNums();
        //printHex();printf("* ");b.printHex();
        ensureEqLen(&b);
        dynsiz prod(ds.size());
        
        int n,m;
        //Double loop like moving diagonally in a matrix to group products
        for(m=0;m<2*ds.size()-1;m++){ //m: index of diagonal
            int ns = ds.size()-iabs(ds.size()-m-1); //ns: count of elements in diagonal
            int ins = ramp(m-ds.size()+1); //index of first element in diagonal (only nonzero for m>=size)
            for(n=0;n<ns;n++){
                //Fx: 0,0  1,0-0,1  2,0-1,1-0,2
                dblword p = ds[ins+n] * b.ds[m-n-ins];
                word p0 = p & wordones;
                word carry = p>>wordbits;//(p-p0) / wordsize;//wordsize==256
                
                /*if(debugbool){
                    printf("m: %d ns: %d ins+n %d m-n-ins: %d\n",m,ns,ins+n,m-n-ins);
                    printf("%d * %d = %d, carry=%d\n",ds[n],b.ds[ns-n-1],p0,carry);
                }*/

                dynsiz addend(m+(carry>0?1:0)+1);
                addend.ds[m] = p0;
                if(carry>0) addend.ds[m+1] = carry;
                if(p0 != 0 || carry != 0)
                    prod+=addend;
            }
        }
        ds.assign(prod.ds.begin(),prod.ds.end());
        //printf("= "); prod.printHex();
    }
    dynsiz operator/(dynsiz d){//integer divison
        //**Ensure d is nonzero
        dynsiz Q(1);//,R(1);
        if(d.eqzero()){//Dividing by zero
            printf("Divided by zero");
            return *this;
        }
        div_remp=new dynsiz(1);
        Q.ds[0]=0; //R.ds[0]=0;
        int j;
        for(int i=ds.size()-1;i>=0;i--){
            //printf("i: %d, ds[i]: %d\n",i,ds[i]);
            for(j=wordbits-1;j>=0;j--){ //for(j=0;j<wordbits;j++){
                //printf("j: %d, bit: %d %d\nR:",j,ds[i] & (1<<j),(ds[i] & (1<<j))>0);
                //(*div_remp).printHex();
                (*div_remp)<<=1;
                //div_remp->printHex();
                (*div_remp).ds[0] |= (ds[i] & (1<<j))>0;
                //div_remp->printHex();
                if(!((*div_remp)<d)){
                    (*div_remp)-=d;
                    Q.ds[0]|=1; //printf("1");
                }//else printf("0");
                //printf("\n");
                if(i>0 || j>0)
                    Q<<=1;
            }
        }
        /*printf("Q: ");
        Q.printHex();
        printf("R: ");
        div_remp->printHex();*/
        return Q;
    }
    dynsiz legacy_divide(dynsiz d){//integer divison
        //**Ensure d is nonzero
        ensureEqLen(&d);
        dynsiz one(1);
        one.ds[0]=1;
        if(d.eqzero()){//Dividing by zero
            printf("Divided by zero");
            return *this;
        }
        div_remp=new dynsiz(ds.size());
        div_remp->ds.assign(ds.begin(),ds.end());
        dynsiz q(1);
        while(!(*div_remp<d)){
            *div_remp-=d;
            q+=one;
        }
        //div_remp->printHex();
        return q;
    }
    dynsiz operator%(dynsiz d){
        operator/(d);
        return *div_remp;
    }
    void digitDiv(dynsiz b,dynsiz base){//Divide number by b and express it as number in given base
        dynsiz q=operator/(b);
        //dynsiz one(1); one.ds[0]=1;
        
        //Finding digits after period
        std::vector<word> fdigits;
        dynsiz r=*div_remp;
        dynsiz r1(1);
        int maxdigits=40,mxdc=0;
        while(!(r.eqzero()) && mxdc<maxdigits){
            r*=base;
            r1=r/b;
            fdigits.push_back(r1.ds[0]);
            r=*(r.div_remp);
            mxdc++;
        }
        
        //Finding digits before period
        std::vector<word> digits;
        dynsiz q1(1);
        do{//do-while loop if this is =0
            q1=q/base; //Save quotient for later (greater digits)
            digits.push_back( q.div_remp->ds[0] ); //Save remainder as digit
            q=q1;
        } while(!(q.eqzero()));//If q positive
        
        printf("digits: ");
        int i;
        for(i=digits.size()-1;i>=0;i--)
            printf("%d",digits[i]);
        if(fdigits.size()>0)
            printf(".");
        for(i=0;i<fdigits.size();i++)
            printf("%d",fdigits[i]);
        printf("\n");
    }
    bool operator<(dynsiz b){
        ensureEqLen(&b);
        for(int i=ds.size()-1;i>=0;i--){
            if(ds[i]==b.ds[i])
                continue;
            else if(ds[i]<b.ds[i])
                return true;
            else
                return false;
        }
        return false; //Equality
    }
    bool operator>(dynsiz b){
        return b<*this;
    }
    bool eqzero(){
        for(int i=0;i<ds.size();i++)
            if(ds[i]!=0)
                return false;
        return true;
    }
};
void primeShortener(dynsiz* nom, dynsiz* denom){
    int primes[]={3,5,7,11,13,17,19,23,29};//2 omitted as denominator is never even
    dynsiz pr(1),nomdiv(1),dendiv(1),nommod(1),denmod(1);
    for(int i=0;i<9;i++){
        pr.ds[0] = primes[i];
        nomdiv=(*nom) / pr;
        dendiv=(*denom) / pr;
        nommod=*(nom->div_remp);
        denmod=*(denom->div_remp);
        //printf("p: %d\n",primes[i]);
        //nommod.printHex();
        //denmod.printHex();
        while( nommod.eqzero() && denmod.eqzero() ){ // If both nominater and denominator divides with prime
            *nom=nomdiv; //(Assignment of) Division of nom by found prime
            *denom=dendiv;
            /*printf("Found p-factor: %d\n",primes[i]);
            nom->printHex();
            denom->printHex();*/
            nomdiv=(*nom) / pr;
            dendiv=(*denom) / pr;
            nommod=*(nom->div_remp);
            denmod=*(denom->div_remp);
            
        }
    }
}
void nilakantha(){
    //Initially: p_nom=3, p_den=1, fac0=2
    dynsiz p_nom(1),p_den(1),add_den(1),fac0(1),fac1(1),fac2(1);
    dynsiz one(1),two(1),four(1);
    one.ds[0]=1; two.ds[0]=2; four.ds[0]=4;
    p_nom.ds[0]=3;
    p_den.ds[0]=1;
    fac0.ds[0]=2;
    
    fac1=fac0; fac1+=one;
    fac2=fac0; fac1+=two;
    for(int i=0;i<5;i++){
        add_den=fac0;
        add_den*=fac1;
        add_den*=fac2;
        fac0.printHex();
        fac1.printHex();
        fac2.printHex();
        add_den.printHex();
        printf("\n");
        fac0+=two;
        fac1+=two;
        fac2+=two;
    }
}
void pi_series(dynsiz f,std::ofstream* ologfile=nullptr){//Wallis product
    //Initially: f=1, nom=2, den=1
    dynsiz p_nom(1),p_den(1),one(1);
    one.ds[0]=1;
    p_nom.ds[0]=2;
    p_den.ds[0]=1;
    
    dynsiz ten(1);
    ten.ds[0]=10;
    
    //main series loop
    for(int i=0;i<200;i++){//64
        printf("i: %d, f: ",i);
        //debugbool=(i==127);
        f.printHex();
        p_den *= f;
        f += one;
        p_nom *= f; p_nom *= f;
        f += one;
        p_den *= f;
        p_nom.printHex();
        p_den.printHex();
        /*printf("----\n");
        f.printNums();
        printf("----\n");*/
        primeShortener(&p_den,&p_nom);
        p_nom.digitDiv(p_den,ten);
    }
}
inline void parseHex2dynsiz(dynsiz* dsptr, std::string s){
    if(s.length()%2==1) s="0"+s;
    //Ensure dsptr->ds.size()==s.length()/2, then uncomment below
    /*
    for(int i=0;i<ds.size();i++)
        dsptr->ds[i]=std::stoi( s.substr(s.length()-2*i-2,2), nullptr, 16);
    */
}
inline void arghandler(int argc, char** argv){
    //argv[0]: path, argv[1]: iterations, argv[2]: file savepath
    std::ifstream ilogfile; //Creating ifstream for loading log (ofstream will be created for same filepath)
    int iter=10; //Default amount of iterations of the pi_series (may be overwritten by argv if given)
    dynsiz p_nom(1),p_den(1),f(1);
    p_den.ds[0]=f.ds[0]=1;
    p_nom.ds[0]=2;
    bool savelog=false;
    for(int i=0;i<argc;i++)
        printf("arg[%d]: %s\n",i,argv[i]);
    
    if(argc>1){
        try{//Check
            iter = std::stoi(argv[1],nullptr,10); // load amount of iterations (dec)
            printf("iter+3=%d\n",iter+3);
        } catch(const std::invalid_argument& ia){
            printf("%s couldn't be passed as an integer",argv[1]);
        }
    }
    if(argc>2){
        ilogfile.open(argv[2]);//std::ios::app, std::ios::ate
        //file layout: nom(hex) den(hex) f(hex)\n
        if(!ilogfile.is_open()){
            printf("Couldn't open %s\n",argv[2]);
            return;
        }
        std::string getl_string,last_line;
        while(getline(ilogfile,getl_string)){//Going through all lines until last
            last_line=getl_string; //Assigning the last line as getl_string will be null after last while-check 
        }
        std::string fa,fb,fc;
        std::istringstream myString(last_line);
        myString>>fa>>fb>>fc; //myString>>std::hex>>fa>>std::hex>>fb>>std::hex>>fc;
        
        std::cout<<(fa)<<" "<<(fb)<<std::endl;
    }
    //pass stream to pi_series (or nullptr)
    ilogfile.close();
}
void div_time_test(){
    //Timing code taken from example
    dynsiz aa(4),bb(4),cc(4);
    bb.ds[0]=3;
    aa.ds[3]=20;
    printf("div_time_test()\naa: ");
    aa.printHex();
    printf("bb: ");
    bb.printHex();
    printf("New method\n");
    const auto t_bef{std::chrono::steady_clock::now()};
    cc=aa/bb;
    const auto t_aft{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{t_aft - t_bef};
    printf("Time: %f seconds\n",elapsed_seconds.count());
    cc.printHex(); aa.div_remp->printHex();
    printf("Old method\n");
    const auto t_bef2{std::chrono::steady_clock::now()};
    cc=aa.legacy_divide(bb);
    const auto t_aft2{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds2{t_aft2 - t_bef2};
    printf("Time: %f seconds\n",elapsed_seconds2.count());
    cc.printHex(); aa.div_remp->printHex();
}
int main(int argc, char** argv){
    //(a+bx+cx²+...)(d+ex+fx²+...)=a(d+ex+fx²)+bx(d+ex+fx²)+cx²(d+ex+fx²)=(ad+aex+afx²)+(bdx+bex²+bfx³)+(cdx²+cex³+cfx⁴)=ad+x(ae+bd)+x²(af+be+cd)+x³(bf+ce)+x⁴cf
    //arghandler(argc,argv); return 0;
    //debugbool=false;
    dynsiz f(1);
    f.ds[0]=1;
    nilakantha(); //pi_series(f);
    return 0;
    dynsiz one(1),two(1);
    one.ds[0]=1; two.ds[0]=2;
    //two/one;
    dynsiz a(4),bbb(3);
    a.ds[3]=0x3b; a.ds[2]=0x9a; a.ds[1]=0xca; a.ds[0]=0x00;
    bbb.ds[2]=0xf; bbb.ds[1]=0x42; bbb.ds[0]=0x40;
    a/bbb;
    return 0;
    a.digitDiv(one,two);
    a<<=2;
    a.digitDiv(one,two);
    a.ds[0]=0xe7;
    a.ds[2]=0xf1;
    a.ds[2]=0x3d;
    a.digitDiv(one,two);
    a<<=6;
    a.digitDiv(one,two);
    return 0;
    //a.ds[1]=44;
    //a.ds[2]=4;
    dynsiz b(3);
    printf("b.eqzero() %d\n",b.eqzero());
    b.ds[0]=185;
    b.ds[1]=0; //b.ds[4]=b.ds[3]=b.ds[2]=
    printf("b.eqzero() %d\n",b.eqzero());
    
    /*a.ds[0]=0xe8; a.ds[1]=3;//2*3*3*5*11
    b.ds[0]=60;//3*5*5*7*11
    a.printHex(); b.printHex();
    primeShortener(&a,&b);
    a.printHex(); b.printHex();*/
    /*a.printNums();
    b.printNums();
    a+=b;
    //a.expandSize(3);
    a.printNums();
    a-=b;
    a.printNums();*/
    //a.un.b=1000;
    //printf("%d %d",a.un.a[0],a.un.a[1]);
    /*a.ds[0]=0xe8; //0xe8;
    a.ds[1]=3; //0x3;
    b.ds[0]=0xe8; //0x90;
    b.ds[1]=3; //0x1;
    b.ds[2]=0;
    a*=b;
    a.printNums();*/
    
    /*dynsiz div0=(a / b);
    div0.printNums();*/
    dynsiz ten(1);
    ten.ds[0]=10;
    b.digitDiv(f,ten);
    //b.div_remp->printNums();
    //(b%f).printNums();
    return 0;
}
