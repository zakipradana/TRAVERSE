#ifndef traverse_h
#define traverse_h

#include <string>
using namespace std;

// Kode jenis kendaraan dan status pemesanan.
#define JUMLAH_JENIS 4
#define MOBIL_PRIBADI 0
#define TRAVEL 1
#define MINIBUS 2
#define BIGBUS 3
#define BELUM_LUNAS 0
#define LUNAS 1
#define BATAL 2

// [STACK] Jenis aksi yang disimpan di stack undo
#define AKSI_KONFIRMASI 0
#define AKSI_BATAL 1
#define AKSI_TAMBAH 2

struct Tanggal {
    int tahun;
    int bulan;
    int hari;
};

struct DataOrang {
    string nama;
    int umur;
    string jk;
};

// MLL cabang 4: anggota rombongan di bawah satu pemesanan
struct Penumpang {
    DataOrang data;
    Penumpang* next;
};

// Riwayat perubahan satu pemesanan
struct Catatan {
    string teks;
    Catatan* next;
};

// MLL cabang 3: pemesanan di bawah satu tempat wisata
struct Pemesanan {
    int idInternal;
    string kodeBooking;
    DataOrang pemesan;
    int jenis;
    string model;
    Tanggal tanggalBerangkat;
    int status;
    Catatan* riwayat;
    Penumpang* daftarPenumpang;
    Pemesanan* next;
};

// MLL cabang 2: tempat wisata di bawah satu kategori
struct TempatWisata {
    string nama;
    Pemesanan* daftarPemesanan;
    TempatWisata* next;
};

// MLL list utama: kategori
struct Kategori {
    string nama;
    TempatWisata* daftarTempat;
    Kategori* next;
};

// [BST] Node AVL untuk indeks. Satu struct dipakai untuk dua pohon:
//       - akarBooking: kunci = kode booking, isi field pemesanan
//       - akarTempat : kunci = "nama (kategori)", isi field tempat
struct NodeBST {
    string kunci;
    Pemesanan* pemesanan;
    TempatWisata* tempat;
    NodeBST* kiri;
    NodeBST* kanan;
    int tinggi;
};

// [QUEUE] Node antrean konfirmasi pembayaran
struct NodeAntrean {
    Pemesanan* data;
    NodeAntrean* next;
};

// [STACK] Node aksi untuk undo
struct Aksi {
    int tipe;
    string deskripsi;
    Pemesanan* p;
    int statusLama;
    int jenisLama;
    string modelLama;
    Aksi* next;
};
//fungsi bantu

int kapasitasJenis(int jenis);
string namaJenis(int jenis);
int jumlahModel(int jenis);
string namaModel(int jenis, int nomor);
bool modelValid(int jenis, string model);
bool jenisUntukKursi(int jumlahKursi, int& hasil);
int kursiAnggota(DataOrang d);
int hitungKursi(Pemesanan* p);
int hitungTercatat(Pemesanan* p);
string namaStatus(int status);
void cetakDetailPemesanan(Pemesanan* p);
Penumpang* sisipAkhirPenumpang(Penumpang* kepala, DataOrang d);
void hapusDaftarPenumpang(Penumpang* kepala);

// Pengelolaan sistem travel menggunakan fungsi prosedural.
void inisialisasiSistem();
void bersihkanSistem();
Kategori* cariKategori(string nama);
TempatWisata* cariTempat(string namaKategori, string namaTempat);       // [BST] pencarian lewat indeks
bool tambahKategori(string nama);
bool tambahTempat(string namaKategori, string namaTempat);             // [BST] disisipkan ke indeks
bool hapusTempat(string namaKategori, string namaTempat, string& pesan); // [BST][STACK][QUEUE] dibersihkan dari semua struktur
void tampilkanKatalog();
int cetakNomorKategori();
string kategoriKe(int nomor);
int cetakNomorTempat(string namaKategori);
string tempatKe(string namaKategori, int nomor);
Pemesanan* cariPemesanan(string kode);                                  // [BST] lewat indeks kode booking
void tampilkanWisataTerurut();                                          // [BST] inorder pada indeks tempat
bool buatPemesanan(string namaKategori, string namaTempat, DataOrang pemesan, Penumpang* anggota, int jenis, string model, Tanggal tanggal, string& kodeHasil, string& pesan); // [BST][QUEUE][H-3]
bool tambahPenumpang(string kode, DataOrang anggota, int jenisBaru, string modelBaru, string& pesan); // [STACK][QUEUE][H-3]
bool konfirmasiBayar(string kode, string& pesan);                       // [QUEUE][H-3]
bool batalkanPemesanan(string kode, string& pesan);                     // [QUEUE]
bool tampilkanPemesananPerWisata(string namaKategori, string namaTempat);
void tampilkanStatistik();

//[QUEUE][STACK][H-3] Antrean konfirmasi, undo, dan jatuh tempo 
// [H-3] Dipanggil sebelum menu: membatalkan otomatis pemesanan belum lunas yang sudah lewat H-3
int jalankanJatuhTempo();
// [QUEUE] Memproses pemesanan terdepan di antrean
bool prosesAntreanKonfirmasi(string& pesan);
// [QUEUE] Menampilkan isi antrean dari depan ke belakang
void tampilkanAntrean();
// [STACK] Membatalkan aksi terakhir
bool undoAksiTerakhir(string& pesan);

#endif