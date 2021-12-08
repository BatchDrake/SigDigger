function X = BitXorMatrix(A,B)
%function to compute the sum without charge of two vectors
	
	%convert elements into usigned integers
	A = uint8(A);
	B = uint8(B);

	m1 = length(A);
	m2 = length(B);
	X = uint8(zeros(m1, m2));
	for n1=1:m1
		for n2=1:m2
			X(n1, n2) = bitxor(A(n1), B(n2));
		end
	end

