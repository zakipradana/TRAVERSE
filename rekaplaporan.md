FITUR YANG TELAH DIBUAT: 
1. Tambah kategori wisata: membuat kategori baru, misalnya Pegunungan atau Pantai.
2. Tambah tempat wisata: menambahkan tempat ke dalam kategori yang dipilih.
3. Tampilkan katalog: menampilkan semua kategori beserta tempat wisatanya.
4. Hapus tempat wisata: menghapus tempat hanya jika tidak ada pemesanan aktif. 
5. Cari tiket: mencari pemesanan berdasarkan kode booking dan menampilkan detailnya.
6. Daftar wisata terurut: menampilkan semua tempat wisata secara alfabetis.
7. Booking: membuat pemesanan baru dengan data pemesan, anggota rombongan, tanggal keberangkatan, dan pilihan kendaraan yang disarankan sistem.
8. Tambah penumpang: menambah anggota ke pemesanan yang sudah ada. Jika kursi tidak cukup, kendaraan diganti dan status lunas kembali menjadi belum lunas.
9. Konfirmasi pembayaran: mengubah status pemesanan dari belum lunas menjadi lunas.
10. Batalkan pemesanan: membatalkan pemesanan, dengan syarat minimal H-3 sebelum keberangkatan.
12. Lihat pemesanan per wisata: menampilkan daftar pemesanan untuk satu tempat wisata.
13. Statistik tempat paling ramai: menghitung pemesanan aktif per tempat wisata dan menampilkan peringkatnya.
14. Keluar: mengakhiri program dan membersihkan memori.

Penambahan fitur:
1. Proses antrean konfirmasi   
2. Lihat antrean konfirmasi           
3. Undo aksi terakhir  

Penambahan ADT:
1. QUEUE untuk antrian konfrimasi pembayaran
2. STACK mengundo konfirmasi pembayaran , pembatalan, dan tambah penumpang.
3. BST untuk mencari tiket dengan kunci kode booking dan daftar wisata terurut.

aturan bisnis baru:
pembayaran wajib h-3 hari keberangakatan, konfirmasi, batalkan  dan tambah penumpang maksimal h-3 hari, Undo hanya bisa dilakukan sampai H-3.

perubahaan pada code: 
traverse.cpp

Ditambahkan state sistem berupa static untuk akar BST, antrean, dan stack.
Ditambahkan fungsi AVL: rotasi, seimbangkan, sisipBST, cariBST, hapusBST, dan cetakInorder.
Ditambahkan fungsi queue: enqueue, hapusDariAntrean, dan sinkronAntrean.
Ditambahkan fungsi stack: catatAksi, bersihkanAksiUntuk, dan konfirmasiInternal.
Ditambahkan aturan H-3 lewat sisaHari dan masihDalamBatas.
Fungsi yang logikanya berubah: inisialisasiSistem, bersihkanSistem, cariTempat, tambahTempat, hapusTempat, cariPemesanan, tampilkanWisataTerurut, buatPemesanan, tambahPenumpang, konfirmasiBayar, dan batalkanPemesanan.
Fungsi baru: jalankanJatuhTempo, prosesAntreanKonfirmasi, tampilkanAntrean, dan undoAksiTerakhir.

traverse.h

Ditambahkan konstanta AKSI_KONFIRMASI, AKSI_BATAL, dan AKSI_TAMBAH.
Ditambahkan struct NodeBST, NodeAntrean, dan Aksi.
Ditambahkan deklarasi jalankanJatuhTempo(), prosesAntreanKonfirmasi(), tampilkanAntrean(), dan undoAksiTerakhir().
Struct data utama (Pemesanan, Penumpang, TempatWisata, Kategori) tidak berubah.


main.cpp

Ditambahkan menu 13, 14, dan 15.
Ditambahkan pemanggilan jalankanJatuhTempo() di awal setiap putaran menu.
Rentang pilihan menu naik dari 0 sampai 12 menjadi 0 sampai 15.
Fungsi dataAwal() dihapus.
Ditambahkan fungsi menuProsesAntrean() dan menuUndo().
