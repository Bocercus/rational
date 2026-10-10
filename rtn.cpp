/*
	Written by Niklas Stentoft Truelsen, 2026
	Except for a few LLM generated lines (see comments)
*/
#include "rtn.h"

inline int iabs(int x) {return (x<0?-x:x);}
inline int ramp(int x) {return (x>0?x:0);}

//bool debugbool;

struct seriesConfig{
	std::fstream* logfile;
	uint16_t iter;
	bool print_all;
	dynsiz base;
	std::vector<dynsiz> inputNumbs;//arrayish
	seriesConfig() : base("0xa") {};// **maybe declare other members?
};

dynsiz::dynsiz(int n){//Init n words filled with zeros
	ds.assign(n,0);
	div_remp=nullptr;
	negat=false;
}
dynsiz::dynsiz(std::string numstr){
	div_remp=nullptr;
	negat=(numstr[0]=='-');
	if(negat){
		numstr=numstr.substr(1,numstr.length()-1);
	}
	if(numstr.substr(0,2)!="0x" || numstr.length()<3){
		printf("Please use hexadecimal and start string with \"0x\"\n");
		ds.assign(1,0); //**Maybe throw exception instead
		return;
	}
	if(numstr.length()%2==1){
		//printf("Uneven length. Turning");
		numstr="0"+numstr.substr(2,numstr.length()-2);//Add initial padding zero for uneven strings and remove 0x
	}else{
		numstr=numstr.substr(2,numstr.length()-2);//Remove 0x
	}
	int L=numstr.length()/2;
	ds.assign(L,0);
	for(int i=0;i<L;i++){
		ds[L-i-1]=std::stoi(numstr.substr(2*i,2),nullptr,16);
	}
}
dynsiz::~dynsiz(){
	delete div_remp;
}
/* AI-written start */
dynsiz::dynsiz(const dynsiz& x){
	//printf("Copy constructor\n");
	ds=x.ds;
	negat=x.negat;
	if(x.div_remp)
		div_remp = new dynsiz(*x.div_remp);
	else
		div_remp = nullptr;
}
 // Copy assignment (strong exception safety)
dynsiz& dynsiz::operator=(const dynsiz& x){
	//printf("Copy assignment\n");
	if (this == &x) return *this;
	ds = x.ds;
	negat = x.negat;
	delete div_remp;
	div_remp = x.div_remp ? new dynsiz(*x.div_remp) : nullptr;
	return *this;
}
// Move constructor
dynsiz::dynsiz(dynsiz&& other) noexcept : ds(std::move(other.ds)), div_remp(other.div_remp), negat{std::exchange(other.negat,0)} {
	//printf("Move constructor\n");
	other.div_remp = nullptr;
}
// Move assignment
dynsiz& dynsiz::operator=(dynsiz&& other) noexcept {
	//printf("Move assignment\n");
	if (this != &other) {
		ds = std::move(other.ds);
		negat = std::exchange(other.negat,0);
		delete div_remp;
		div_remp = other.div_remp;
		other.div_remp = nullptr;
	}
	return *this;
}
/* AI-written end */
bool dynsiz::expandSize(int n){
	for(int i=0;i<n;i++)
		ds.push_back(0);
	return true;
}
void dynsiz::printNums(){
	if(negat)
		printf("-");
	for(uint16_t i=0;i<ds.size();i++)
		printf("%d ",ds[i]); //%x
	printf("\n");
}
void dynsiz::printHex(){
	if(negat)
		printf("-");
	printf("0x");
	for(uint16_t i=0;i<ds.size();i++){
		if(ds[ds.size()-i-1]<16)
			printf("0");
		printf("%x",ds[ds.size()-i-1]);
	}
	printf("\n");
}
dynsiz dynsiz::getDSsign(){
	return negat?dynsiz("0x01"):dynsiz("0x00");
}
void dynsiz::ensureEqLen(dynsiz* bptr){//Ensure pointed synsize have equal size as this object
	if(bptr->ds.size()>ds.size()) expandSize(bptr->ds.size()-ds.size());
	if(bptr->ds.size()<ds.size()) bptr->expandSize(ds.size()-bptr->ds.size());
	//Appearently returning a bool from this functions breaks something (when void->bool ofc)
}
void dynsiz::operator<<=(word bs){//bs: amount of bits to be shifted
	if(bs>wordbits){//We shall not shift more than an extra word
		printf("Bitshift too large (%d)\n",bs);
		// **maybe throw exception
		return;
	}
	std::vector<dblword> bsds;
	uint16_t i;
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
	if(bsds[ds.size()-1] & carryones){ //If carry for most-significant bit is unzero add bit
		expandSize(1);
		ds[ds.size()-1] = (bsds[ds.size()-2]>>wordbits);
	}
}
inline void dynsiz::pm(dynsiz b,bool plus){//private
	ensureEqLen(&b);
	dblword sum;
	word c=0;
	if(b.negat){//Turning negative sign of b into a change in operator and sign of b
		b.negat = false;
		plus = !plus;
	}
	if(negat){//If this ("a") is negative change operator but not overall sign
		plus = !plus;
	}
	uint16_t i;
	for(i=0;i<ds.size();i++){
		if(plus){//Addition
			sum=ds[i]+b.ds[i]+c;
			c=sum>>wordbits; //Carry
		}else{//Subtraction
			sum=ds[i]-b.ds[i]-c; //Difference (sum, if b is negative)
			c=(sum>>wordbits) & 1; //The result is 0xffxx for subtraction overflow
		}
		//printf("sum: %d, c: %d\n",sum,c);
		ds[i]=sum & wordones;
	}
	if(c==1){//overflow
		if(plus){
			expandSize(1);
			ds[ds.size()-1]=1;
		}else{
			negat = !negat;
			for(i=0;i<ds.size();i++)
				ds[i] ^= wordones;//not on all bits
			dynsiz carry_one("0x1"); carry_one.negat=negat;
			operator+=(carry_one);
			//printf("**Negative number\n");
		}
	}
}
void dynsiz::operator+=(dynsiz& b){ pm(b,true); return;
	/* //Appearently this is slower:
	ensureEqLen(&b);
	dblword sum;
	word c=0;
	for(int i=0;i<ds.size();i++){
		sum=ds[i]+b.ds[i]+c;
		c=sum>>wordbits;
		ds[i]=sum & 0xff;
	}
	if(c==1){
		expandSize(1);
		ds[ds.size()-1]=1;
	}
	*/
}
void dynsiz::operator-=(dynsiz& b){ pm(b,false); return;
	//** consider subtraction when this<b
	/* //Appearently this is slower:
	ensureEqLen(&b);
	dblword sum;
	word c=0;
	for(int i=0;i<ds.size();i++){
		sum=ds[i]-b.ds[i]-c; //Difference (sum, if b is negative)
		c=(sum>>wordbits) & 1; //The result is 0xffxx for subtraction overflow
		//printf("sum: %d, c: %d\n",sum,c);
		ds[i]=sum & 0xff;
	}
	if(c==1){
		printf("**Negative number\n");
	}*/
}
void dynsiz::operator*=(dynsiz& b){
	ensureEqLen(&b);
	dynsiz prod(ds.size());

	uint16_t n,m;
	//Double loop like moving diagonally in a matrix to group products
	for(m=0;m<2*ds.size()-1;m++){ //m: index of diagonal
		uint16_t ns = ds.size()-iabs(ds.size()-m-1); //ns: count of elements in diagonal
		uint16_t ins = ramp(m-ds.size()+1); //index of first element in diagonal (only nonzero for m>=size)
		for(n=0;n<ns;n++){
			//Fx: 0,0  1,0-0,1  2,0-1,1-0,2
			dblword p = ds[ins+n] * b.ds[m-n-ins];
			word p0 = p & wordones;
			word carry = p>>wordbits;

			dynsiz addend(m+(carry>0?1:0)+1);
			addend.ds[m] = p0;
			if(carry>0) addend.ds[m+1] = carry;
			if(p0 != 0 || carry != 0)
				prod+=addend;
		}
	}
	ds.assign(prod.ds.begin(),prod.ds.end());
	negat ^= b.negat; //Set sign
}
dynsiz dynsiz::operator/(dynsiz d){//integer divison
	//**Ensure d is nonzero
	dynsiz Q(1);//,R(1);
	if(d.eqzero()){//Dividing by zero
		printf("Divided by zero");
		// **maybe throw exception
		return *this;
	}
	div_remp=new dynsiz(1); //Pointer to remainder, **Maybe associate it with quotient rather than dividend
	int j;
	for(int i=ds.size()-1;i>=0;i--){
		//printf("i: %d, ds[i]: %d\n",i,ds[i]);
		for(j=wordbits-1;j>=0;j--){ //for(j=0;j<wordbits;j++){
			//printf("j: %d, bit: %d %d\nR:",j,ds[i] & (1<<j),(ds[i] & (1<<j))>0);
			(*div_remp)<<=1;
			(*div_remp).ds[0] |= (ds[i] & (1<<j))>0; //Assign jth bit of ds to least sig. bit of remainder
			if(!((*div_remp)<d)){
				(*div_remp)-=d;
				Q.ds[0]|=1; //printf("1");
			}
			if(i>0 || j>0)
				Q<<=1;
		}
	}
	Q.negat = negat ^ d.negat;
	return Q;
}
dynsiz dynsiz::legacy_divide(dynsiz d){//integer divison
	//**Ensure d is nonzero
	ensureEqLen(&d);
	dynsiz one(1);
	one.ds[0]=1;
	if(d.eqzero()){//Dividing by zero
		printf("Divided by zero");
		// **maybe throw exception
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
dynsiz dynsiz::operator%(dynsiz d){
	operator/(d);
	return *div_remp;
}
std::string dynsiz::digitDiv(dynsiz b,dynsiz base,int maxdigits=20,bool unaryPlus=false){//Divide number by b and express it as number in given base
	std::string out_str;
	ensureEqLen(&b);

	//Finding digits after period
	std::vector<word> fdigits;
	dynsiz q=operator/(b); //Actual integer division
	dynsiz r=*div_remp;
	dynsiz r1(1);
	int mxdc=0; // Max digit counter (second)
	while(!(r.eqzero()) && mxdc<maxdigits){//Generate until no digits left or maxdigit digits have been generated
		r*=base;
		r1=r/b; //Finding digit through quotient ...
		fdigits.push_back(r1.ds[0]);
		r=*(r.div_remp); //... and using the remainder to find the later digits
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

	if((negat ^ b.negat)){
		out_str+="-";
	} else if(unaryPlus)
		out_str+="+";
	if(digits.size()>0)
		for(std::vector<word>::reverse_iterator i=digits.rbegin();i!=digits.rend();i++) //Append from last element in digits
			out_str+=std::format("{:x}",*i);
	else
		out_str+='0';
	uint16_t i;
	if(fdigits.size()>0)
		out_str+='.';
	for(i=0;i<fdigits.size();i++)
		out_str+=std::format("{:x}",fdigits[i]); //printf("%d",fdigits[i]);
	//printf("\n");
	return out_str;
}
bool dynsiz::operator<(dynsiz b){
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
bool dynsiz::operator>(dynsiz b){
	return b<*this;
}
bool dynsiz::eqzero(){
	for(uint16_t i=0;i<ds.size();i++)
		if(ds[i]!=0)
			return false;
	return true;
}
//**write these file functions: get first n entries, get last n entries, append given entries
//Structure of (binary) logfiles: [size of entry][null-terminator][entry]{repeat for each entry}
std::vector<uint8_t> uint32_to_utf8(uint32_t num){
	std::vector<uint8_t> out_arr;
	//printf("num: %d\n",num);
	if(num>=(1<<21)){//UTF-8 stores max 21 bits (afaik)
		printf("Number too large! %d\n",num);
		// **maybe throw exception
	} else if(num>=(1<<16)){//Max 21-bits -> 4 UTF-8 bytes: 11110uvv 10vvwwww 10xxxxyy 10yyzzzz
		out_arr={0xF0,0x80,0x80,0x80};
		out_arr[3] |= static_cast<uint8_t>(num) & 0x3F; //LSB
		out_arr[2] |= static_cast<uint8_t>(num>>6) & 0x3F;
		out_arr[1] |= static_cast<uint8_t>(num>>12) & 0x3F;
		out_arr[0] |= static_cast<uint8_t>(num>>18); //MSB
	} else if(num>=(1<<11)){//Max 16-bits -> 3 UTF-8 bytes: 1110wwww 10xxxxyy 10yyzzzz
		out_arr={0xE0,0x80,0x80};
		out_arr[2] |= static_cast<uint8_t>(num) & 0x3F;
		out_arr[1] |= static_cast<uint8_t>(num>>6) & 0x3F;
		out_arr[0] |= static_cast<uint8_t>(num>>12);
	} else if(num>=(1<<7)){//Max 11-bits -> 2 UTF-8 bytes: 110xxxyy 10yyzzzz
		out_arr={0xC0,0x80};
		out_arr[1] |= static_cast<uint8_t>(num) & 0x3F;
		out_arr[0] |= static_cast<uint8_t>(num>>6);
	} else {//Max 7-bits -> 1 UTF-8 byte: 0yyyzzzz
		out_arr={static_cast<uint8_t>(num)};
	}
	return out_arr;
}

inline void append_entries2f(std::fstream* file, dynsiz* entries, int ent_arr_size){
	//**ensure correct endian-ness
	file->seekp(0,std::ios_base::end);
	std::streampos f_pos = file->tellg();
	//std::print("File size before: {}\n",static_cast<uint16_t>(f_pos));
	uint32_t ent_size=0;//dynsiz ent_size(1),one("0x1");
	std::vector<uint8_t> siz_out_arr;
	for(int i=0;i<ent_arr_size;i++){
		ent_size = entries[i].ds.size();
		siz_out_arr=uint32_to_utf8(ent_size);//uint8_t siz_out_arr[]={static_cast<uint8_t>(ent_size),static_cast<uint8_t>(ent_size>>8)};
		file->write(reinterpret_cast<char*>(siz_out_arr.data()),siz_out_arr.size());
		file->write(reinterpret_cast<char*>(entries[i].ds.data()),ent_size); //**8-bit
		//For 16-bit use something like: file->write(reinterpret_cast<char*>({static_cast<uint8_t>(v),static_cast<uint8_t>(v>>8)}),2);
	}
	file->seekp(0,std::ios_base::end);
	f_pos = file->tellg();
	//std::print("File size after: {}\n",static_cast<uint16_t>(f_pos));
	file->flush();//**test this
}
std::vector<dynsiz> extract_entriesOF(std::fstream* file, uint16_t entries, int startpos=0, bool relative=false){
	//startpos=-1 means extract the entries from end of file
	std::vector<dynsiz> ext_arr;
	std::deque<std::streampos> pos_stack; // **std::fpos vs. std::streampos
	//pos_stack.reserve(entries);
	std::deque<uint32_t> siz_stack;
	//siz_stack.reserve(entries);
	std::streampos cur_pos, end_pos;
	cur_pos = file->tellg();
	file->seekg(0, std::ios_base::end);
	end_pos = file->tellg();
	if(startpos<-1){
		printf("startpos must be greather than -2\n");
		// **maybe throw exception
		return ext_arr;
	}
	if(relative)
		file->seekg(cur_pos);
	else
		file->seekg(0, std::ios_base::beg);

	//std::print("typeid(abcde) {}",typeid(abcde).name()); // \# include <typeinfo>
	char* buf;
	uint32_t siz;
	uint8_t extra_bytes;
	uint16_t e=0;
	while(e<entries && (file->tellg())<end_pos){ //Here the positions and sizes of the entry data are found
		//Get size byte(s) from UTF-8 format
		buf=new char[1];
		file->read(buf, 1);
		extra_bytes=0;
		//printf("First byte: %x %x %x %x\n",buf[0],buf[0] & 0xE0,buf[0] & 0xF0,buf[0] & 0xF8);
		//First byte determines amount of extra bytes for the size
		if((buf[0] & 0x80)==0){//1 UTF-8 byte: 0xxxxxxx
			siz=buf[0];
		} else if((buf[0] & 0xE0)==0xC0){//2 UTF-8 bytes: 110xxxxx ...
			siz=buf[0] & 0x1F;
			//printf("buf[0] & 0x1F: %d\n",buf[0] & 0x1F);
			extra_bytes=1;
		} else if((buf[0] & 0xF0)==0xE0){//3 UTF-8 bytes: 1110xxxx ...
			siz=buf[0] & 0xF;
			extra_bytes=2;
		} else if((buf[0] & 0xF8)==0xF0){//4 UTF-8 bytes: 11110xxx ...
			siz=buf[0] & 0x7;
			extra_bytes=2;
		} else {
			printf("Byte doesn't seem to be UTF-8: %x\n",buf[0]);
		}
		for(uint8_t i=0;i<extra_bytes;i++){
			file->read(buf, 1);
			if((buf[0] & 0xC0) != 0x80){
				printf("Following byte not equal 0b10xxxxxx: %x\n",buf[0]);
				// **maybe throw exception
			}
			siz<<=6;
			siz |= buf[0] & 0x3F;
		}
		delete[] buf;
		//**pop
		if(siz_stack.size()>=entries){
			pos_stack.pop_front();
			siz_stack.pop_front();
		}
		pos_stack.push_back(file->tellg()); //Save start position of entry data
		siz_stack.push_back(siz);
		file->seekg(siz, std::ios_base::cur); //Skip to next entry size info

		//std::streampos f_pos = file->tellg();
		//std::print("size: {}, pos: {}\n",siz,static_cast<uint16_t>(f_pos));

		if(entries!=-1 && startpos==0)
			e++;
		else if(startpos>0)
			startpos--;
	}
	/*std::println("while term for entries {} startpos {} relative {}",entries,startpos,relative);
	for(int j : pos_stack)
		std::println("pos {}",j);
	for(int j : siz_stack)
		std::println("siz {}",j);*/
	for(e=0;e<siz_stack.size();e++){
		file->seekg(pos_stack[e]);
		buf=new char[siz_stack[e]];
		file->read(buf, siz_stack[e]);
		dynsiz ds_buf(siz_stack[e]);
		std::copy(buf, buf+siz_stack[e], ds_buf.ds.data());
		ext_arr.push_back( ds_buf );
		//ds_buf.printHex();
		delete[] buf;
	}
	return ext_arr;
}
void primeShortener(dynsiz* num, dynsiz* denom){
	word primes[]={2,3,5,7,11,13,17,19,23,29};//((2 omitted as denominator is never even))
	dynsiz pr(1),numdiv(1),dendiv(1),nummod(1),denmod(1);
	for(int i=0;i<10;i++){
		pr.ds[0] = primes[i];
		numdiv=(*num) / pr;
		dendiv=(*denom) / pr;
		nummod=*(num->div_remp);
		denmod=*(denom->div_remp);
		//printf("p: %d\n",primes[i]);
		//nummod.printHex();
		//denmod.printHex();
		while( nummod.eqzero() && denmod.eqzero() ){ // If both numerator and denominator divides with prime
			*num=numdiv; //(Assignment of) Division of num by found prime
			*denom=dendiv;
			/*printf("Found p-factor: %d\n",primes[i]);
			num->printHex();
			denom->printHex();*/
			numdiv=(*num) / pr;
			dendiv=(*denom) / pr;
			nummod=*(num->div_remp);
			denmod=*(denom->div_remp);
		}
	}
}
void primeShortenerThread(dynsiz n,dynsiz d,word prime,std::atomic<uint16_t>& fac){//This is only meant to be called by primeShortenerPar
	dynsiz pr(1),ndiv(1),ddiv(1),nmod(1),dmod(1);
	pr.ds[0]=prime;
	ndiv=n / pr;
	ddiv=d / pr;
	nmod=*(n.div_remp);
	dmod=*(d.div_remp);
	while( nmod.eqzero() && dmod.eqzero() ){
		n=ndiv;
		d=ddiv;
		fac+=1; //Found factor
		ndiv=n / pr;
		ddiv=d / pr;
		nmod=*(n.div_remp);
		dmod=*(d.div_remp);
	}
}
void primeShortenerPar(dynsiz* num, dynsiz* denom){//Parallelized version of primeShortener (can be exchanged 1:1)
	//Divides numerator and denominator of fraction by greatest common divisor
	word primes[]={2,3,5,7,11,13,17,19,23,29};
	std::vector<std::thread> threads;
	threads.reserve(10);
	std::array<std::atomic<uint16_t>,10> facs{{{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}}}; //How many times each prime is a factor
	for(int i=0;i<10;i++)
		threads.push_back(std::thread(primeShortenerThread,*num,*denom,primes[i],std::ref(facs[i])));
	for(auto& th : threads) th.join();

	//Divide by the found factors
	uint16_t j;
	dynsiz pr(1);
	for(int i=0;i<10;i++){
		j=facs[i].load(std::memory_order_relaxed);
		pr.ds[0] = primes[i];
		while(j>0){
			*num = *num / pr;
			*denom = *denom / pr;
			j--;
		}
	}
}
void exponential(seriesConfig* sc){
	//**Exponential: ∑_{n=0}^∞ x^n/n! = ∑_{n=0}^∞ [(xnr+xni*i)/xd]^n/n!
	//**xd must be nonzero, **sqrt(1/2)~xni=0xb,xd=0x3
	dynsiz resnr(1),resni(1),resd(1); //result real numerator, result imaginary numerator, result denominator
	dynsiz xnr(1),xni(1),xd(1); //base real numerator, base imaginary numerator, base denominator
	dynsiz add_numr(1),add_numi(1),add_den(1),n(1),fac(1),one("0x1"); //Added fraction and factorial
	dynsiz semi_prod0(1),semi_prod1(1),add_den_fac(1);
	/*These should be saved:
		Initially: xnr, xni, xd
		For each iteration: resnr, resni, resd, add_numr, add_numi, add_den, fac, n*/
	std::chrono::steady_clock::time_point clock_before, clock_after;

	xnr     =sc->inputNumbs[0];
	xni     =sc->inputNumbs[1];
	xd      =sc->inputNumbs[2];
	resnr   =sc->inputNumbs[3]; //default: 1
	resni   =sc->inputNumbs[4]; //0
	resd    =sc->inputNumbs[5]; //1
	add_numr=sc->inputNumbs[6]; //1
	add_numi=sc->inputNumbs[7]; //0
	add_den =sc->inputNumbs[8]; //1
	fac     =sc->inputNumbs[9]; //1
	n       =sc->inputNumbs[10]; //1

	std::println("Complex exponential ({}{}i)/{}\nInitial values:\n Numerator(r): {}\n Numerator(i): {}\n  Denominator: {}",xnr.digitDiv(one,sc->base),xni.digitDiv(one,sc->base,20,true),xd.digitDiv(one,sc->base),resnr.digitDiv(one,sc->base),resni.digitDiv(one,sc->base),resd.digitDiv(one,sc->base));
	//**avoid "+-" for negative imaginary part^^
	if(sc->logfile!=nullptr) std::println("Will save log."); else std::println("Will not save log.");
	clock_before = std::chrono::steady_clock::now();
	//Assuming the "sum" already has been done for n=0
	for(uint16_t i=0;i<sc->iter;i++){
		//Finding [(xnr+xni*i)/xd]^n :
		// (a+bi)(c+di)=(ac+adi+bci-bd)
		// add_numr=add_numr*xnr-add_numi*xni
		// add_numi=add_numr*xni+add_numi*xnr
		semi_prod0=add_numi; semi_prod0*=xni;
		semi_prod1=add_numr; semi_prod1*=xni;
		add_numr*=xnr; add_numr-=semi_prod0;
		add_numi*=xnr; add_numi+=semi_prod1;
		add_den*=xd;
		//Factorial:
		fac*=n; n+=one;
		add_den_fac=add_den; add_den_fac*=fac; //Assigning this to new variable so add_den can represent xd^n for the following iterations
		//Adding fraction to result:
		// (a+bi)/e + (c+di)/f = (af+bfi+ce+dei)/ef
		// resnr=resnr*add_den+resd*add_numr;
		// resni=resni*add_den+resd*add_numi;
		semi_prod0=add_numr; semi_prod0*=resd;
		semi_prod1=add_numi; semi_prod1*=resd;
		resnr*=add_den_fac; resnr+=semi_prod0;
		resni*=add_den_fac; resni+=semi_prod1;
		resd*=add_den_fac;
		if(sc->print_all){
			std::print("{}{}i = ",resnr.digitDiv(resd,sc->base),resni.digitDiv(resd,sc->base,20,true));
			std::println("({}{}i) / {}",resnr.digitDiv(one,sc->base),resni.digitDiv(one,sc->base,20,true),resd.digitDiv(one,sc->base));
		}
		if(sc->logfile!=nullptr){//<resnr><resni><resd><add_numr><add_numi><add_den><fac><n><resnr_sign><resni_sign><add_numr_sign><add_numi_sign>
			dynsiz s_a[] = {resnr, resni, resd, add_numr, add_numi, add_den, fac, n, resnr.getDSsign(), resni.getDSsign(), add_numr.getDSsign(), add_numi.getDSsign()};
			append_entries2f(sc->logfile, s_a, 12);
		}
	}
	clock_after = std::chrono::steady_clock::now();
	if(sc->iter>0){
		std::chrono::duration<float> elapsed_time = clock_after - clock_before;
		std::println("Calculated {} term(s) in {} seconds.",sc->iter,elapsed_time.count());
	}
	std::print("{}{}i = ",resnr.digitDiv(resd,sc->base),resni.digitDiv(resd,sc->base,20,true));
	std::println("({}{}i) / {}",resnr.digitDiv(one,sc->base),resni.digitDiv(one,sc->base,20,true),resd.digitDiv(one,sc->base));
}
void nilakantha(seriesConfig* sc){
	//π = 3+∑_{n=0}^∞ (-1)^n*4/([2n+2][2n+3][2n+4])
	//N/D+a/b = Nb/Db+Da/Db = (Nb+Da)/Db
	dynsiz p_num(1),p_den(1),fac0(1); //Initially: p_num=3, p_den=1, fac0=2
	p_num=sc->inputNumbs[0];
	p_den=sc->inputNumbs[1];
	fac0=sc->inputNumbs[2];
	dynsiz add_num(1),add_den(1),fac1(1),fac2(1);
	dynsiz one("0x1"),two("0x2"),four("0x4");
	word f0_2parity = (fac0 % four).ds[0];
	std::chrono::steady_clock::time_point clock_before, clock_after; // **Maybe: using namespace std::chrono;
	//using namespace std::chrono; steady_clock::time_point clock_begin,clock_end; clock_begin = steady_clock::now();
	//double testpi=3.0;
	//double testn=2.0;

	fac1=fac0; fac1+=one;
	fac2=fac0; fac2+=two;

	std::println("Nilakantha series (pi)\nInitial values:\n    Numerator: {}\n  Denominator: {}",p_num.digitDiv(one,sc->base),p_den.digitDiv(one,sc->base));
	std::println("     Fraction: {}",p_num.digitDiv(p_den,sc->base));
	if(sc->logfile!=nullptr) std::println("Will save log."); else std::println("Will not save log.");
	clock_before = std::chrono::steady_clock::now();
	//main series loop
	for(uint16_t i=0;i<sc->iter;i++){// 3/1, 19/6, 47/15, 1321/420,
		add_den=fac0;
		add_den*=fac1;
		add_den*=fac2;
		add_num=p_den; add_num*=four;
		p_num*=add_den; p_den*=add_den;
		//printf("add_num: "); add_num.printHex();
		//printf("add_den: "); add_den.printHex();
		if(f0_2parity!=0)//**Ensure correct parity if an uneven number was saved
			p_num+=add_num;
		else
			p_num-=add_num;
		f0_2parity = 2 - f0_2parity; //Switching between adding/subtracting
		primeShortenerPar(&p_den,&p_num);
		if(sc->print_all)
			std::println("{} = {} / {}",p_num.digitDiv(p_den,sc->base),p_num.digitDiv(one,sc->base),p_den.digitDiv(one,sc->base));//Write as decimal fraction

		fac0+=two;
		fac1+=two;
		fac2+=two;

		if(sc->logfile!=nullptr){//Save numerator, denominator and factor0 to logfile
			append_entries2f(sc->logfile, &p_num, 1);
			append_entries2f(sc->logfile, &p_den, 1);
			append_entries2f(sc->logfile, &fac0, 1);
		}

		//printf("%.18f\n",((i%2==0)?1:-1)*4.0/(testn*(testn+1.0)*(testn+2.0)));
		/*testpi+=((i%2==0)?1:-1)*4.0/(testn*(testn+1.0)*(testn+2.0));
		testn+=2.0;
		printf("%da:digits: %.18f\n",i,testpi);*/
	}
	//Print timing:
	clock_after = std::chrono::steady_clock::now();
	if(sc->iter>0){
		std::chrono::duration<float> elapsed_time = clock_after - clock_before;
		std::println("Calculated {} term(s) in {} seconds.",sc->iter,elapsed_time.count());
	}
	std::print("{} = ",p_num.digitDiv(p_den,sc->base));
	std::println("{} / {}",p_num.digitDiv(one,sc->base),p_den.digitDiv(one,sc->base));
	//printf("%s\n",p_num.digitDiv(p_den,sc->base).c_str());
}
void wallis(seriesConfig* sc){//Wallis product
	dynsiz p_num(1),p_den(1),f(1); //Initially: num=2, den=1, f=1
	p_num=sc->inputNumbs[0];
	p_den=sc->inputNumbs[1];
	f=sc->inputNumbs[2];

	dynsiz one("0x1");
	std::chrono::steady_clock::time_point clock_before, clock_after;
	std::println("Wallis product (pi)\nInitial values:\n    Numerator: {}\n  Denominator: {}",p_num.digitDiv(one,sc->base),p_den.digitDiv(one,sc->base));
	std::println("     Fraction: {}",p_num.digitDiv(p_den,sc->base));
	if(sc->logfile!=nullptr) std::println("Will save log."); else std::println("Will not save log.");

	clock_before = std::chrono::steady_clock::now();
	//main series loop
	for(uint16_t i=0;i<sc->iter;i++){
		//printf("i: %d, f: ",i);
		p_den *= f;
		f += one;
		p_num *= f; p_num *= f;
		f += one;
		p_den *= f;
		primeShortenerPar(&p_den,&p_num);
		if(sc->print_all)
			std::println("{} = {} / {}",p_num.digitDiv(p_den,sc->base),p_num.digitDiv(one,sc->base),p_den.digitDiv(one,sc->base));
		if(sc->logfile!=nullptr){//Save numerator, denominator and factor0 to logfile
			append_entries2f(sc->logfile, &p_num, 1);
			append_entries2f(sc->logfile, &p_den, 1);
			append_entries2f(sc->logfile, &f, 1);
		}
	}
	clock_after = std::chrono::steady_clock::now();
	if(sc->iter>0){
		std::chrono::duration<float> elapsed_time = clock_after - clock_before;
		std::println("Calculated {} term(s) in {} seconds.",sc->iter,elapsed_time.count());
	}
	std::print("{} = ",p_num.digitDiv(p_den,sc->base));
	std::println("{} / {}",p_num.digitDiv(one,sc->base),p_den.digitDiv(one,sc->base));
}
inline void arghandler(int argc, char** argv){
	/* Flags:
		-A: Algorithm
			-A e: Complex exponential function
			-A n: Nilakantha
			-A w: Walli's
		-a, --all: output all to screen (and file), otherwise only first and last
		-b, --base: set number base of printed output
		-h, --help: help
		-i, --iterations: iterations of algorithm
		-f, --filename: output file name
	*/
	//**Maybe add power series a^b = ∑_{n=0}^∞ (b choose n)*(x-1)^n, where (b choose n)=b*(b-1)(b-2)...(b-n+1)/n!
	//**Maybe for -d set i=0 and let all algos print results after loop
	//**Maybe add -e for only saving last iteration
	//File layout: "v<version_num><algoChar>[...]"
	seriesConfig sc;
	std::fstream logfile; //Creating fstream for loading and saving log ((((ofstream will be created for same filepath))))
	std::streampos lf_ps_before, lf_ps_after;
	sc.iter=1; //Default amount of iterations of the series (may be overwritten by argv if given)
	sc.logfile=&logfile;
	char algoChar=-1;
	int16_t algoParams[3]={0};
	//enum {none=-1, euler, nilakantha, wallis} algo;
	sc.print_all=false;
	std::string filename="";
	enum eFlags {
		flag_algo, flag_all, flag_base, flag_help, flag_iter, flag_filename
	};
	std::map<std::string,eFlags> str2flag={
		{"-A",flag_algo},
		{"-a",flag_all},{"--all",flag_all},
		{"-b",flag_base},{"--base",flag_base},
		{"-h",flag_help},{"--help",flag_help},
		{"-i",flag_iter},{"--iterations",flag_iter},
		{"-f",flag_filename},{"--filename",flag_filename}
	};
	const char* help_msg="Flags:\n"
	"-A: Algorithm\n"
	"	-A e: Complex exponential function\n"
	"	-A n: Nilakantha\n"
	"	-A w: Walli's\n"
	"-a, --all: output all to screen (and file), otherwise only first and last\n"
	"-b, --base: set number base of printed output (default value: 2*5)\n"
	"-h, --help: help\n"
	"-i, --iterations: iterations of algorithm (in decimal, default value: 1)\n"
	"-f, --filename: output file name";

	if(argc<2){
		printf("%s\n",help_msg); return;
	}

	for(int i=1;i<argc;i++){//argv[0] is skipped
		switch(str2flag[argv[i]]){
			case flag_algo:
				if(i+1<argc){ //If parameter has been specified
					if(algoChar!=-1){
						printf("Multiple algorithms set\n");
						return;
					}
					algoChar=argv[i+1][0];
					i++; //Skip over read char
					if(algoChar=='e'){//Reading parameters for complex exponential algorithm
						if(argc<=i+3){
							printf("Please write real numerator, imaginary numerator, and denominator of exponential base\n"); return;
						}
						try{
							algoParams[0] = std::stoi(argv[i+1],nullptr,10);
							algoParams[1] = std::stoi(argv[i+2],nullptr,10);
							algoParams[2] = std::stoi(argv[i+3],nullptr,10);
							i+=3;
						}catch(const std::invalid_argument& ia){
							printf("Error passing numbers following -A e\n"); return;
						}
					}
				}
			break;
			case flag_all:
				sc.print_all=true;
			break;
			case flag_base:
				if(i+1<argc){
					try{
						uint16_t b_input;
						b_input = std::stoi(argv[i+1],nullptr,10); // load amount of iterations (dec)
						if(b_input<2){
							printf("base must be greater than 1\n"); return;
						} else if(b_input>16){
							printf("base must be less than 17\n"); return;
						}
						sc.base.ds[0] = b_input;
						i++;
					} catch(const std::invalid_argument& ia){
						printf("%s couldn't be passed as an integer\n",argv[i+1]); return;
					}
				}else{
					printf("Please specify number after \"-b\"\n");
					return;
				}
			break;
			case flag_help:
				printf("%s\n",help_msg); return;
			break;
			case flag_iter:
				if(i+1<argc){ //If parameter has been specified
					try{//Check if parameter is indeed a number
						uint32_t i_input = std::stoi(argv[i+1],nullptr,10); // load amount of iterations (dec)
						if(i_input>0xffff){
							printf("iterations must be non-negative and less than 65536\n"); return;
						}
						sc.iter = i_input;
					} catch(const std::invalid_argument& ia){
						printf("%s couldn't be passed as an integer\n",argv[i+1]); return;
					}
					i++; //Skip over read number
				}else{
					printf("Please specify number after \"-i\"\n");
					return;
				}
				//printf("iter+3=%d\n",sc.iter+3);
			break;
			case flag_filename:
				if(i+1<argc){ //If parameter has been specified
					filename=argv[i+1];
					i++; //Skip over read filename
				}else{
					printf("Please specify filename after \"-f\"");
					return;
				}
			break;
			default:
				printf("Unknown flag: %s\n",argv[i]);
		}
	}

	bool log_alreadyx = false;
	if(filename!=""){
		std::filesystem::path logfilepath = filename;
		log_alreadyx = std::filesystem::exists(logfilepath);
		logfile.open(filename,std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::app);
		if(!logfile.is_open()){
			printf("Error loading file\n");
			return;
		}
		logfile.seekp(0, std::ios_base::end);
		lf_ps_before=logfile.tellp();
		dynsiz pv_ds(p_version);
		if(log_alreadyx){
			//**Set sc.logfile using file
			std::vector<dynsiz> x = extract_entriesOF(&logfile, 3);
			/*x[0].printHex();
			x[1].printHex();
			x[2].printHex();*/
			if(x.size()==0){
				printf("Unable to read from file\n");
				logfile.close(); return;
			}
			if(x[0].ds.size()!=1 || x[0].ds[0]!=0x76){
				printf("Initial entry not v (version)\n");
				logfile.close(); return;
				//**exit
			}
			/*
			if(x[1].ds.size()!=1 || x[1].ds[0]!=p_version){
				printf("Version mismatch\n");
				// **exit
			}*/
			if(x[2].ds.size()!=1){
				printf("AlgoChar size mismatch\n");
				logfile.close(); return;
				//**exit
			}
			algoChar = x[2].ds[0];
			std::println("Successfully opened existing file: {}",filename);
		} else if(algoChar!=-1){
			//Init file
			std::println("Creating new file: {}",filename);
			dynsiz initout[]={dynsiz("0x76"), pv_ds, dynsiz(1)}; //Adding: "v<version_num><algoChar>"
			initout[2].ds[0] = algoChar;
			append_entries2f(&logfile, initout, 3);
		}
		//dynsiz ftestout[]={dynsiz(4096)};
		//ftestout[0].ds[384]=5; ftestout[0].ds[3]=34;
		//append_entries2f(&logfile, ftestout, 1);

		/*dynsiz ftestout[]={dynsiz(260), dynsiz("0x5ef")};
		append_entries2f(&logfile, ftestout, 2);
		extract_entriesOF(&logfile, 1);*/

		//printf("fs_ex: %d\n",fs_ex);
	} else
		sc.logfile = nullptr;
	if(algoChar==-1){
		printf("No algorithm found/chosen\n");
		logfile.close(); return;
	}

	switch(algoChar){
		case 'e': //Complex exponential function
			//File layout: "v<version_num>e<xnr><xni><xd><xnr_sign><xni_sign><resnr><resni><resd><add_numr><add_numi><add_den><fac><n><resnr_sign><resni_sign><add_numr_sign><add_numi_sign><resnr><resni>[...]"
			if(log_alreadyx){
				//Get parameters from file
				std::vector<dynsiz> x = extract_entriesOF(&logfile, 5, 3); //extract_entriesOF(std::fstream* file, uint16_t entries, int startpos=0, bool relative=false)
				std::vector<dynsiz> y = extract_entriesOF(&logfile, 12, -1);
				//**if(x.size()<**)
				sc.inputNumbs.push_back(dynsiz(x[0])); //xnr
				sc.inputNumbs.push_back(dynsiz(x[1])); //xni
				sc.inputNumbs.push_back(dynsiz(x[2])); //xd
				sc.inputNumbs.push_back(dynsiz(y[0])); //resnr
				sc.inputNumbs.push_back(dynsiz(y[1])); //resni
				sc.inputNumbs.push_back(dynsiz(y[2])); //resd
				sc.inputNumbs.push_back(dynsiz(y[3])); //add_numr
				sc.inputNumbs.push_back(dynsiz(y[4])); //add_numi
				sc.inputNumbs.push_back(dynsiz(y[5])); //add_den
				sc.inputNumbs.push_back(dynsiz(y[6])); //fac
				sc.inputNumbs.push_back(dynsiz(y[7])); //n
				sc.inputNumbs[0].negat = x[3].ds[0]; //xnr_sign
				sc.inputNumbs[1].negat = x[4].ds[0]; //xni_sign
				sc.inputNumbs[3].negat = y[8].ds[0]; //resnr_sign
				sc.inputNumbs[4].negat = y[9].ds[0]; //resni_sign
				sc.inputNumbs[6].negat = y[10].ds[0]; //add_numr_sign
				sc.inputNumbs[7].negat = y[11].ds[0]; //add_numi_sign
				// **Ensure xd, resd, add_den, fac and n are non-negative
			} else {
				//From scratch
				if(algoParams[2]==0){
					printf("Exponential base denominator must be nonzero");
					logfile.close(); return;
				}
				if(algoParams[2]<0){
					algoParams[0]*=-1;
					algoParams[1]*=-1;
					algoParams[2]*=-1;
				}
				std::string OX="0x";
				sc.inputNumbs.push_back(dynsiz(((algoParams[0]<0)?"-":"")+OX+std::format("{:x}",iabs(algoParams[0])))); //xnr
				sc.inputNumbs.push_back(dynsiz(((algoParams[1]<0)?"-":"")+OX+std::format("{:x}",iabs(algoParams[1])))); //xni
				sc.inputNumbs.push_back(dynsiz(OX+std::format("{:x}",algoParams[2]))); //xd
				sc.inputNumbs.push_back(dynsiz("0x01")); //resnr
				sc.inputNumbs.push_back(dynsiz(1)); //resni
				sc.inputNumbs.push_back(dynsiz("0x01")); //resd
				sc.inputNumbs.push_back(dynsiz("0x01")); //add_numr
				sc.inputNumbs.push_back(dynsiz(1)); //add_numi
				sc.inputNumbs.push_back(dynsiz("0x01")); //add_den
				sc.inputNumbs.push_back(dynsiz("0x01")); //fac
				sc.inputNumbs.push_back(dynsiz("0x01")); //n
				if(sc.logfile!=nullptr){
					append_entries2f(sc.logfile, sc.inputNumbs.data(), 3);
					dynsiz xnr_sign=sc.inputNumbs[0].getDSsign();
					dynsiz xni_sign=sc.inputNumbs[1].getDSsign();
					append_entries2f(sc.logfile, &xnr_sign, 1);
					append_entries2f(sc.logfile, &xni_sign, 1);
				}
			}
			exponential(&sc);
		break;
		case 'n': //Nilakantha series
			//File layout: "v<version_num>n<p_num><p_den><fac0><p_num><p_den>[...]"
			if(log_alreadyx){//File exists
				std::vector<dynsiz> x = extract_entriesOF(&logfile, 3, -1); //Get: p_num, p_den, fac0
				if(x.size()<3){
					printf("Expected 3 entries after header entries. Only found: %lu\n",x.size());
					logfile.close(); return;
				}
				sc.inputNumbs.push_back(x[0]);
				sc.inputNumbs.push_back(x[1]);
				sc.inputNumbs.push_back(x[2]);
			} else {
				//From scratch
				sc.inputNumbs.push_back(dynsiz("0x3")); //p_num
				sc.inputNumbs.push_back(dynsiz("0x1")); //p_den
				sc.inputNumbs.push_back(dynsiz("0x2")); //fac0
				if(sc.logfile!=nullptr){
					//Adding initNumbs to logfile
					append_entries2f(sc.logfile, sc.inputNumbs.data(), 3);
				}
			}
			nilakantha(&sc);
		break;
		case 'w': //Walli's product
			//File layout: "v<version_num>w<p_num><p_den><f><p_num><p_den>[...]"
			if(log_alreadyx){//File exists
				std::vector<dynsiz> x = extract_entriesOF(&logfile, 3, -1); //Get: p_num, p_den, f
				if(x.size()<3){
					printf("Expected 3 entries after header entries. Only found: %lu\n",x.size());
					logfile.close(); return;
				}
				sc.inputNumbs.push_back(x[0]);
				sc.inputNumbs.push_back(x[1]);
				sc.inputNumbs.push_back(x[2]);
			} else {
				//From scratch
				sc.inputNumbs.push_back(dynsiz("0x2")); //p_num
				sc.inputNumbs.push_back(dynsiz("0x1")); //p_den
				sc.inputNumbs.push_back(dynsiz("0x1")); //f
				if(sc.logfile!=nullptr){
					//Adding initNumbs to logfile
					append_entries2f(sc.logfile, sc.inputNumbs.data(), 3);
				}
			}
			wallis(&sc);
		break;
		default:
			printf("Unknown algorithm: %c\n",algoChar);
	}
	if(sc.logfile!=nullptr){
		logfile.seekp(0, std::ios_base::end);
		lf_ps_after=logfile.tellp();
		std::streamoff file_app = lf_ps_after - lf_ps_before;
		if(file_app>0){
			std::println("Wrote {} byte(s) to {}",file_app,filename);
		} else {
			std::println("Nothing was written to {}",filename);
		}
	}
	logfile.close();

	//Walli's init
	/*dynsiz p_num(1),p_den(1),f(1);
	p_den.ds[0]=f.ds[0]=1;
	p_num.ds[0]=2;*/

	/*for(int i=0;i<argc;i++)
		printf("arg[%d]: %s\n",i,argv[i]);

	if(argc>1){}
	if(argc>2){
		ilogfile.open(argv[2]);//std::ios::app, std::ios::ate
		//file layout: num(hex) den(hex) f(hex)\n
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
	//pass stream to series (or nullptr)
	i*/
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
	printf("C++ version: %ld\n",__cplusplus);
	if(__cplusplus < 202302)
		printf("Please use C++23 (or newer)");
	/*dynsiz ps_a("0x24c"),ps_b("0x3f");
	ps_a.printHex(); ps_b.printHex();
	primeShortenerPar(&ps_a,&ps_b);
	ps_a.printHex(); ps_b.printHex();*/

	/*for(uint8_t i=0;i<1;i++){
		dynsiz ps_c0("-0x1000"),ps_d0("0xc"); ps_c0.printHex(); ps_d0.printHex();
		ps_c0+=ps_d0; ps_c0.printHex();
		dynsiz ps_c1("0xc4"),ps_d1("-0x20c4"); ps_c1.printHex(); ps_d1.printHex();
		ps_c1+=ps_d1; ps_c1.printHex();
		dynsiz ps_c2("-0x10"),ps_d2("0xc"); ps_c2.printHex(); ps_d2.printHex();
		ps_c2-=ps_d2; ps_c2.printHex();
		dynsiz ps_c3("-0xc"),ps_d3("0x10"); ps_c3.printHex(); ps_d3.printHex();
		ps_c3-=ps_d3; ps_c3.printHex();
	}*/
	//exponential();
	//return 0;
	//(a+bx+cx²+...)(d+ex+fx²+...)=a(d+ex+fx²)+bx(d+ex+fx²)+cx²(d+ex+fx²)=(ad+aex+afx²)+(bdx+bex²+bfx³)+(cdx²+cex³+cfx⁴)=ad+x(ae+bd)+x²(af+be+cd)+x³(bf+ce)+x⁴cf
	/*uint32_t ab=0b11111111111;
	std::vector<uint8_t> out;
	out=uint32_to_utf8(ab);
	for(int i=0;i<out.size();i++){
		printf("%x\n",out[i]);
	}
	return 0;*/
	/*std::fstream logfile;
	logfile.open("./that_pi",std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::app);
	if(!logfile.is_open()){
		printf("Error loading file\n");
		return 0;
	}
	extract_entriesOF(&logfile, 3);
	extract_entriesOF(&logfile, 2, 0, true);
	extract_entriesOF(&logfile, 2, 1);
	extract_entriesOF(&logfile, 4, -1);
	extract_entriesOF(&logfile, 64, 4);
	logfile.close();
	return 0;*/
	arghandler(argc,argv); return 0;
	//debugbool=false;
	//nilakantha(); //wallis(f);
	return 0;
	dynsiz aa("0xeFA0"); aa.printHex();
	dynsiz f("0x1");
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