#include "pi0.h"

inline int iabs(int x) {return (x<0?-x:x);}
inline int ramp(int x) {return (x>0?x:0);}

class dynsiz;

//bool debugbool;

/*struct seriesConfig{
	std::fstream* blabla;
	uint8_t iter;
	bool all;
	dynsiz inputstuff;//arrayish
};*/

dynsiz::dynsiz(int n){//Init n words filled with zeros
	ds.assign(n,0);
	div_remp=nullptr;
}
dynsiz::dynsiz(std::string numstr){
	div_remp=nullptr;
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
	delete div_remp;
	div_remp = x.div_remp ? new dynsiz(*x.div_remp) : nullptr;
	return *this;
}
// Move constructor
dynsiz::dynsiz(dynsiz&& other) noexcept : ds(std::move(other.ds)), div_remp(other.div_remp) {
	//printf("Move constructor\n");
	other.div_remp = nullptr;
}
// Move assignment
dynsiz& dynsiz::operator=(dynsiz&& other) noexcept {
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
bool dynsiz::expandSize(int n){
	for(int i=0;i<n;i++)
		ds.push_back(0);
	return true;
}
void dynsiz::printNums(){
	for(uint16_t i=0;i<ds.size();i++)
		printf("%d ",ds[i]); //%x
	printf("\n");
}
void dynsiz::printHex(){
	printf("0x");
	for(uint16_t i=0;i<ds.size();i++){
		if(ds[ds.size()-i-1]<16)
			printf("0");
		printf("%x",ds[ds.size()-i-1]);
	}
	printf("\n");
}
void dynsiz::ensureEqLen(dynsiz* bptr){//Ensure pointed synsize have equal size as this object
	if(bptr->ds.size()>ds.size()) expandSize(bptr->ds.size()-ds.size());
	if(bptr->ds.size()<ds.size()) bptr->expandSize(ds.size()-bptr->ds.size());
	//Appearently returning a bool from this functions breaks something (when void->bool ofc)
}
void dynsiz::operator<<=(word bs){//bs: amount of bits to be shifted
	if(bs>wordbits){//We shall not shift more than an extra word
		printf("Bitshift too large (%d)\n",bs);
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
	for(uint16_t i=0;i<ds.size();i++){
		if(plus){
			sum=ds[i]+b.ds[i]+c;
			c=sum>>wordbits;
		}else{
			sum=ds[i]-b.ds[i]-c; //Difference (sum, if b is negative)
			c=(sum>>wordbits) & 1; //The result is 0xffxx for subtraction overflow
		}
		//printf("sum: %d, c: %d\n",sum,c);
		ds[i]=sum & 0xff;
	}
	if(c==1){
		if(plus){
			expandSize(1);
			ds[ds.size()-1]=1;
		}else{
			printf("**Negative number\n");
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
}
dynsiz dynsiz::operator/(dynsiz d){//integer divison
	//**Ensure d is nonzero
	dynsiz Q(1);//,R(1);
	if(d.eqzero()){//Dividing by zero
		printf("Divided by zero");
		return *this;
	}
	div_remp=new dynsiz(1); //Pointer to remainder, **Maybe associate it with quotient rather than dividend
	Q.ds[0]=0;
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
	return Q;
}
dynsiz dynsiz::legacy_divide(dynsiz d){//integer divison
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
dynsiz dynsiz::operator%(dynsiz d){
	operator/(d);
	return *div_remp;
}
std::string dynsiz::digitDiv(dynsiz b,dynsiz base,int maxdigits=20){//Divide number by b and express it as number in given base
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

	if(digits.size()>0)
		for(std::vector<word>::reverse_iterator i=digits.rbegin();i!=digits.rend();i++) //Append from last element in digits
			out_str+=std::to_string(*i);
	else
		out_str+='0';
	uint16_t i;
	if(fdigits.size()>0)
		out_str+='.';
	for(i=0;i<fdigits.size();i++)
		out_str+=std::to_string(fdigits[i]); //printf("%d",fdigits[i]);
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
	printf("num: %d\n",num);
	if(num>=(1<<21)){//UTF-8 stores max 21 bits (afaik)
		printf("Number too large! %d\n",num);
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
	std::print("File size before: {}\n",static_cast<uint16_t>(f_pos));
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
	std::print("File size after: {}\n",static_cast<uint16_t>(f_pos));
	file->flush();//**test this
}
std::vector<dynsiz> extract_entriesOF(std::fstream* file, int entries){
	file->seekg(0, std::ios_base::beg);
	char* buf;
	uint32_t siz;
	uint8_t extra_bytes;
	std::vector<dynsiz> ext_arr;
	for(int e=0;e<entries;e++){
		//Get size byte(s) from UTF-8 format
		buf=new char[1];
		file->read(buf, 1);
		extra_bytes=0;
		//printf("First byte: %x %x %x %x\n",buf[0],buf[0] & 0xE0,buf[0] & 0xF0,buf[0] & 0xF8);
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
			}
			siz<<=6;
			siz |= buf[0] & 0x3F;
		}
		std::streampos f_pos = file->tellg();
		std::print("size: {}, pos: {}\n",siz,static_cast<uint16_t>(f_pos));
		delete[] buf;
		buf=new char[siz];
		file->read(buf, siz);
		dynsiz ds_buf(siz);
		std::copy(buf, buf+siz, ds_buf.ds.data());
		ext_arr.push_back( ds_buf );
		//ds_buf.printHex();
		delete[] buf;
	}
	return ext_arr;
}
void primeShortener(dynsiz* nom, dynsiz* denom){
	int primes[]={2,3,5,7,11,13,17,19,23,29};//((2 omitted as denominator is never even))
	dynsiz pr(1),nomdiv(1),dendiv(1),nommod(1),denmod(1);
	for(int i=0;i<10;i++){
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
	//π = 3+∑_{n=0}^∞ (-1)^n*4/([2n+2][2n+3][2n+4])
	//N/D+a/b = Nb/Db+Da/Db = (Nb+Da)/Db
	dynsiz p_nom("0x3"),p_den("0x1"),fac0("0x2");
	dynsiz add_nom(1),add_den(1),fac1(1),fac2(1);
	dynsiz one("0x1"),two("0x2"),four("0x4");
	//double testpi=3.0;
	//double testn=2.0;

	fac1=fac0; fac1+=one;
	fac2=fac0; fac2+=two;

	dynsiz ten(1); ten.ds[0]=10;
	for(int i=0;i<200;i++){// 3/1, 19/6, 47/15, 1321/420,
		add_den=fac0;
		add_den*=fac1;
		add_den*=fac2;
		add_nom=p_den; add_nom*=four;
		p_nom*=add_den; p_den*=add_den;
		//printf("add_nom: "); add_nom.printHex();
		//printf("add_den: "); add_den.printHex();
		if(i%2==0)
			p_nom+=add_nom;
		else
			p_nom-=add_nom;
		primeShortener(&p_den,&p_nom);
		printf("%d: ",i);
		//std::print("{} / {}: ",p_nom.digitDiv(one,ten),p_den.digitDiv(one,ten));//Write as decimal fraction

		fac0+=two;
		fac1+=two;
		fac2+=two;
		//printf("%.18f\n",((i%2==0)?1:-1)*4.0/(testn*(testn+1.0)*(testn+2.0)));
		std::println("{}",p_nom.digitDiv(p_den,ten));//(previously) weird i's: 22, 53 (now fixed)
		//printf("%s\n",p_nom.digitDiv(p_den,ten).c_str()); //**Probably bad practice, the package <print> seem to fix it using std::print from C++23
		/*testpi+=((i%2==0)?1:-1)*4.0/(testn*(testn+1.0)*(testn+2.0));
		testn+=2.0;
		printf("%da:digits: %.18f\n",i,testpi);*/
	}
	std::println("{}",p_nom.digitDiv(p_den,ten));
	//printf("%s\n",p_nom.digitDiv(p_den,ten).c_str());
}
void wallis(dynsiz f,std::ofstream* ologfile=nullptr){//Wallis product
	//Initially: f=1, nom=2, den=1
	dynsiz p_nom("0x2"),p_den("0x1"),one("0x1");

	dynsiz ten(1);
	ten.ds[0]=10;

	//main series loop
	for(int i=0;i<100;i++){//64
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
		//printf("%s\n",p_nom.digitDiv(p_den,ten).c_str());
		std::println("{}",p_nom.digitDiv(p_den,ten));
	}
}
inline void arghandler(int argc, char** argv){
	/* Flags:
		-A: Algorithm
			-A e: Euler's number
			-A n: Nilakantha
			-A w: Walli's
		-a, --all: output all to screen (and file), otherwise only first and last
		((-d: display most recent value in file))
		-h, --help: help
		-i, --iterations: iterations of algorithm
		-f, --filename: output file name
	*/
	//**Add timer, maybe instead use: using namespace std::chrono; steady_clock::time_point clock_begin,clock_end; clock_begin = steady_clock::now();
	//**Maybe add power series a^b = ∑_{n=0}^∞ (b choose n)*(x-1)^n, where (b choose n)=b*(b-1)(b-2)...(b-n+1)/n!
	//**Exponential: ∑_{n=0}^∞ x^n/n!
	//**Maybe for -d set i=0 and let all algos print results after loop
	std::fstream logfile; //Creating fstream for loading and saving log ((((ofstream will be created for same filepath))))
	int iter=1; //Default amount of iterations of the series (may be overwritten by argv if given)
	char algoChar=-1;
	//enum {none=-1, euler, nilakantha, wallis} algo;
	//algo=none;
	bool savelog=false; //**Currently unused
	bool outp_all=false; //**Currently unused
	std::string filename="";
	enum eFlags {
		flag_algo, flag_all, flag_help, flag_iter, flag_filename
	};
	std::map<std::string,eFlags> str2flag={
		{"-A",flag_algo},
		{"-a",flag_all},{"--all",flag_all},
		{"-h",flag_help},{"--help",flag_help},
		{"-i",flag_iter},{"--iterations",flag_iter},
		{"-f",flag_filename},{"--filename",flag_filename}
	};
	const char* help_msg="Flags:\n\
	-A: Algorithm\n\
		-A e: Euler's number\n\
		-A n: Nilakantha\n\
		-A w: Walli's\n\
	-a, --all: output all to screen (and file), otherwise only first and last\n\
	((-d: display most recent value in file))\n\
	-h, --help: help\n\
	-i, --iterations: iterations of algorithm\n\
	-f, --filename: output file name";

	for(int i=1;i<argc;i++){//argv[0] is skipped
		switch(str2flag[argv[i]]){
			case flag_algo:
				if(i+1<argc){ //If parameter has been specified
					algoChar=argv[i+1][0];
					i++; //Skip over read char
				}
			break;
			case flag_all:
				outp_all=true;
			break;
			case flag_help:
				printf("%s\n",help_msg);
			break;
			case flag_iter:
				if(i+1<argc){ //If parameter has been specified
					try{//Check if parameter is indeed a number
						iter = std::stoi(argv[i+1],nullptr,10); // load amount of iterations (dec)
					} catch(const std::invalid_argument& ia){
						printf("%s couldn't be passed as an integer",argv[i+1]);
					}
					i++; //Skip over read number
				}else{
					printf("Please specify number after \"-i\"");
				}
				printf("iter+3=%d\n",iter+3);
			break;
			case flag_filename:
				if(i+1<argc){ //If parameter has been specified
					filename=argv[i+1];
					i++; //Skip over read filename
				}else{
					printf("Please specify filename after \"-f\"");
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
		if(log_alreadyx){
			/*char* ftestbuf=new char[4];
			logfile.read(ftestbuf,4);
			for(int i=0;i<4;i++){
				printf("%d ",ftestbuf[i]);
			}
			printf("\n");
			delete[] ftestbuf;*/
		} else {}
		//dynsiz ftestout[]={dynsiz(4096)};
		//ftestout[0].ds[384]=5; ftestout[0].ds[3]=34;
		//append_entries2f(&logfile, ftestout, 1);

		dynsiz ftestout[]={dynsiz(260), dynsiz("0x5ef")};
		append_entries2f(&logfile, ftestout, 2);
		extract_entriesOF(&logfile, 1);

		//printf("fs_ex: %d\n",fs_ex);
	}

	switch(algoChar){
		case 'e':
			//algo=euler;
			printf("Euler's\n");
		break;
		case 'n':
			//algo=nilakantha;
			printf("Nilakantha's\n");
		break;
		case 'w':
			//algo=wallis;
			printf("Walli's\n");
		break;
		default:
			printf("Unknown algorithm: %c\n",algoChar);
	}
	logfile.close();

	//Walli's init
	/*dynsiz p_nom(1),p_den(1),f(1);
	p_den.ds[0]=f.ds[0]=1;
	p_nom.ds[0]=2;*/

	/*for(int i=0;i<argc;i++)
		printf("arg[%d]: %s\n",i,argv[i]);

	if(argc>1){}
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
	printf("C++ version: %d\n",__cplusplus);
	//(a+bx+cx²+...)(d+ex+fx²+...)=a(d+ex+fx²)+bx(d+ex+fx²)+cx²(d+ex+fx²)=(ad+aex+afx²)+(bdx+bex²+bfx³)+(cdx²+cex³+cfx⁴)=ad+x(ae+bd)+x²(af+be+cd)+x³(bf+ce)+x⁴cf
	/*uint32_t ab=0b11111111111;
	std::vector<uint8_t> out;
	out=uint32_to_utf8(ab);
	for(int i=0;i<out.size();i++){
		printf("%x\n",out[i]);
	}
	return 0;*/
	//arghandler(argc,argv); dynsiz aa("0xeFA0"); aa.printHex(); return 0;
	//debugbool=false;
	nilakantha(); //wallis(f);
	return 0;
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