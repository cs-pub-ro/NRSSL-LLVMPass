// Decode NRS bit patterns with NRSSL, the library the pass uses to encode them.
//
//   java -cp nrssl.jar:scala-library.jar NrsDecode.java <type> <bits>
//
// Reads one hex bit pattern per line from stdin and prints its value as a
// double, or "nan" if NRSSL can't decode it.

import java.io.BufferedReader;
import java.io.InputStreamReader;
import ro.upb.nrs.sl.*;

public class NrsDecode {
    static double decode(String type, String bin, int size) {
        switch (type) {
        case "posit1":
            return Posit$.MODULE$.apply(bin, 2, size, Posit$.MODULE$.default_rounding()).toDouble();
        case "posit2":
            return Posit$.MODULE$.apply(bin, 3, size, Posit$.MODULE$.default_rounding()).toDouble();
        case "morris":
            return Morris$.MODULE$.apply(bin, morrisG(size), size, Morris$.MODULE$.default_rounding()).toDouble();
        case "morrisHeb":
            return MorrisHEB$.MODULE$.apply(bin, morrisG(size), size, MorrisHEB$.MODULE$.default_rounding()).toDouble();
        case "morrisBiasHeb":
            return MorrisBiasHEB$.MODULE$.apply(bin, morrisG(size), size, MorrisBiasHEB$.MODULE$.default_rounding()).toDouble();
        case "morrisUnaryHeb":
            return MorrisUnaryHEB$.MODULE$.apply(bin, size, MorrisUnaryHEB$.MODULE$.default_rounding()).toDouble();
        default:
            throw new IllegalArgumentException("unknown type " + type);
        }
    }

    // same sizes as NRSSL.h in the pass
    static int morrisG(int size) {
        switch (size) {
        case 8: return 2;
        case 16: return 3;
        case 32: return 4;
        default: return 6;
        }
    }

    public static void main(String[] args) throws Exception {
        String type = args[0];
        int size = Integer.parseInt(args[1]);
        BufferedReader in = new BufferedReader(new InputStreamReader(System.in));
        for (String line; (line = in.readLine()) != null;) {
            line = line.trim();
            if (line.isEmpty())
                continue;
            long v = Long.parseUnsignedLong(line.replaceFirst("^0x", ""), 16);
            StringBuilder bin = new StringBuilder(Long.toBinaryString(v));
            while (bin.length() < size)
                bin.insert(0, '0');
            double d;
            try {
                d = decode(type, bin.toString(), size);
            } catch (RuntimeException e) {
                d = Double.NaN;
            }
            System.out.println(Double.isNaN(d) ? "nan" : Double.toString(d));
        }
    }
}
